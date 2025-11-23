#include "common.h"
#include "daemon.h"
#include "backup.h"
#include "ipc.h"
#include "file.h"
#include "log.h"

#include <signal.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <poll.h>
#include <dirent.h>

#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN (1024 * (INOTIFY_EVENT_SIZE + 16))

static volatile sig_atomic_t running = 1;
static MonitorState *g_state = NULL;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    if (g_state) {
        ILOG("Received signal, shutting down gracefully\n");
    }
}

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        ELOG("Failed to fork process\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(0);
    }

    umask(0);
    setsid();
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        if (null_fd > 2) {
            close(null_fd);
        }
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
}

char* get_process_cwd(pid_t pid) {
    static char cwd_path[MAX_PATH_LEN] = "";
    char proc_path[256] = "";

    ssize_t len;

    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", pid);
    len = readlink(proc_path, cwd_path, sizeof(cwd_path) - 1);
    if (len != -1) {
        cwd_path[len] = '\0';
        return cwd_path;
    } else {
        ELOG("Failed to readlink /proc/%d/cwd\n", pid);
        return NULL;
    }
}

int init_monitor_state(MonitorState *state, pid_t pid) {

    state->monitored_pid        = pid;
    state->sample_period_ms     = DEFAULT_SAMPLE_PERIOD_MS;
    state->current_sample       = 0;
    state->first_backup_done    = false;
    state->file_count           = 0;
    state->capacity             = MAX_FILES;
    state->inotify_fd           = -1;
    state->watch_list           = NULL;
    state->watch_count          = 0;
    state->watch_capacity       = 0;
    state->changed_files_queue  = NULL;
    state->changed_files_count  = 0;
    state->files                = (FileInfo*)calloc(state->capacity, sizeof(FileInfo));

    if (state->files == NULL) {
        ELOG("Failed to allocate memory for file info\n");
        return -1;
    }

    char *cwd = get_process_cwd(pid);
    if (cwd == NULL) {
        ELOG("Failed to get CWD for PID %d\n", pid);
        return -1;
    }

    state->monitored_dir = strdup(cwd);
    if (state->monitored_dir == NULL) {
        ELOG("Failed to duplicate CWD string\n");
        return -1;
    }

    ILOG("Initialized monitor state: PID=%d, dir=%s\n", pid, cwd);
    return 0;
}

void cleanup_monitor_state(MonitorState *state) {
    cleanup_inotify(state);

    if (state->files) {
        free(state->files);
        state->files = NULL;
    }

    if (state->monitored_dir) {
        free(state->monitored_dir);
        state->monitored_dir = NULL;
    }

    if (state->watch_list) {
        for (size_t i = 0; i < state->watch_count; i++) {
            free(state->watch_list[i].path);
        }
        free(state->watch_list);
        state->watch_list = NULL;
    }

    if (state->changed_files_queue) {
        for (size_t i = 0; i < state->changed_files_count; i++) {
            free(state->changed_files_queue[i]);
        }
        free(state->changed_files_queue);
        state->changed_files_queue = NULL;
    }

    free(state);
}

const char* get_watch_path(MonitorState *state, int wd) {
    for (size_t i = 0; i < state->watch_count; i++) {
        if (state->watch_list[i].wd == wd) {
            return state->watch_list[i].path;
        }
    }

    return NULL;
}

int add_inotify_watch_recursive(MonitorState *state, const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        ELOG("failed to open dir: %s", path);
        return -1;
    }

    // Add watch for current directory
    int wd = inotify_add_watch(state->inotify_fd, path,
                    IN_MODIFY       | IN_CREATE     | IN_DELETE |
                    IN_MOVED_FROM   | IN_MOVED_TO   | IN_CLOSE_WRITE);

    if (wd < 0) {
        ELOG("Failed to add watch for %s\n", path);
        closedir(dir);
        return -1;
    }

    // Store watch descriptor and path
    if (state->watch_count >= state->watch_capacity) {
        size_t new_capacity = state->watch_capacity == 0 ? 32 : state->watch_capacity * 2;

        WatchInfo *new_list = (WatchInfo*)realloc(state->watch_list, new_capacity * sizeof(WatchInfo));
        if (new_list == NULL) {
            ELOG("Failed to realloc watch list\n");
            closedir(dir);
            return -1;
        }

        state->watch_list = new_list;
        state->watch_capacity = new_capacity;
    }

    state->watch_list[state->watch_count].wd    = wd;
    state->watch_list[state->watch_count].path  = strdup(path);

    if (state->watch_list[state->watch_count].path == NULL) {
        ELOG("Failed to strdup path\n");
        closedir(dir);
        return -1;
    }

    state->watch_count++;

    struct dirent *entry    = NULL;
    char fullpath[PATH_MAX] = "";

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // If d_type is DT_UNKNOWN, we need to check, but for directories we can try to open
        if (entry->d_type == DT_DIR) {
            // Recursively add watch for subdirectories
            add_inotify_watch_recursive(state, fullpath);
        } else if (entry->d_type == DT_UNKNOWN) {
            // If d_type unknown, try to open dir!
            DIR *test_dir = opendir(fullpath);
            if (test_dir != NULL) {
                closedir(test_dir);
                add_inotify_watch_recursive(state, fullpath);
            }
        }
    }

    closedir(dir);
    return 0;
}

int init_inotify(MonitorState *state) {
    // IN_NONBLOCK -> read operations on this descriptor will not block,
    // even if there are no events   <---
    //                                  |
    state->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (state->inotify_fd < 0) {
        ELOG("Failed to initialize inotify\n");
        return -1;
    }

    if (add_inotify_watch_recursive(state, state->monitored_dir) != 0) {
        ELOG("Failed to add inotify watches\n");
        close(state->inotify_fd);
        state->inotify_fd = -1;
        return -1;
    }

    ILOG("Inotify initialized for directory: %s\n", state->monitored_dir);
    return 0;
}

void cleanup_inotify(MonitorState *state) {
    if (state->inotify_fd >= 0) {
        close(state->inotify_fd);
        state->inotify_fd = -1;
    }
}


int process_inotify_events(MonitorState *state) {
    if (state->inotify_fd < 0) {
        return -1;
    }

    char buffer[INOTIFY_BUF_LEN] = "";
    ssize_t length = read(state->inotify_fd, buffer, INOTIFY_BUF_LEN);

    if (length < 0) {
        if (errno == EAGAIN) {
            return 0;
        }
        ELOG("Failed to read from inotify: %s\n", strerror(errno));
        return -1;
    }

    size_t i = 0;
    size_t len = (size_t)length;
    while (i < len) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];

        if (event->len > 0) {
            // Get the directory path for this watch descriptor
            const char *watch_dir = get_watch_path(state, event->wd);
            if (watch_dir == NULL) {
                // Watch descriptor not found, skip this event
                i += (size_t)INOTIFY_EVENT_SIZE + (size_t)event->len;
                continue;
            }

            // Build full path to the file/directory
            char file_path[PATH_MAX] = "";
            snprintf(file_path, sizeof(file_path), "%s/%s", watch_dir, event->name);

            // Check if it's a directory using IN_ISDIR flag (no stat needed!)
            bool is_dir = (event->mask & IN_ISDIR) != 0;

            // Handle directory creation - add watch for new directories
            if (is_dir && (event->mask & IN_CREATE)) {
                DLOG("Directory created: %s\n", file_path);
                add_inotify_watch_recursive(state, file_path);
                i += (size_t)INOTIFY_EVENT_SIZE + (size_t)event->len;
                continue;
            }

            // Only process regular files (not directories)
            if (!is_dir) {
                // Skip log files created by the daemon itself
                const char *basename = strrchr(file_path, '/');
                if (basename != NULL) {
                    basename++; // Skip the '/'
                } else {
                    basename = file_path;
                }

                // Ignore debug.log and other log files in monitored directory
                if (strcmp(basename, "debug.log") == 0) {
                    i += (size_t)INOTIFY_EVENT_SIZE + (size_t)event->len;
                    continue;
                }

                bool should_add     = false;
                bool is_text        = false;

                // For file events, check if it's a text file only when file exists
                // (CREATE, MODIFY, CLOSE_WRITE) - not for DELETE
                if (event->mask & (IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO)) {
                    // File exists, can check if it's text
                    is_text = is_text_file(file_path);
                } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                    // File doesn't exist anymore, but we still need to track it
                    // Use the path from queue or state if available
                    is_text = true; // Assume it was a text file if we're tracking it
                }

                if (is_text) {
                    if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
                        should_add = true;
                        DLOG("File modified: %s\n", file_path);
                    } else if (event->mask & IN_CREATE) {
                        should_add = true;
                        DLOG("File created: %s\n", file_path);
                    } else if (event->mask & IN_DELETE) {
                        should_add = true;
                        DLOG("File deleted: %s\n", file_path);
                    } else if (event->mask & (IN_MOVED_FROM | IN_MOVED_TO)) {
                        should_add = true;
                        DLOG("File moved: %s\n", file_path);
                    }

                    if (should_add) {
                        // Check if already in queue
                        bool found = false;
                        for (size_t j = 0; j < state->changed_files_count; j++) {
                            if (strcmp(state->changed_files_queue[j], file_path) == 0) {
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            state->changed_files_queue = (char**)realloc(
                                state->changed_files_queue,
                                (state->changed_files_count + 1) * sizeof(char*));
                            if (state->changed_files_queue != NULL) {
                                state->changed_files_queue[state->changed_files_count] = strdup(file_path);
                                if (state->changed_files_queue[state->changed_files_count] != NULL) {
                                    state->changed_files_count++;
                                }
                            }
                        }
                    }
                }
            }
        }

        i += (size_t)INOTIFY_EVENT_SIZE + (size_t)event->len;
    }

    return 0;
}

int run_daemon(MonitorState *state) {

    if (init_ipc() != 0) {
        ELOG("Failed to initialize IPC\n");
        return -1;
    }

    if (init_inotify(state) != 0) {
        ELOG("Failed to initialize inotify\n");
        cleanup_ipc();
        return -1;
    }

    ILOG("Daemon started, monitoring PID %d in directory %s\n",
         state->monitored_pid, state->monitored_dir);
    ILOG("Note: debug.log files are excluded from monitoring\n");

    // Create first full backup
    if (create_full_backup(state) != 0) {
        WLOG("Failed to create initial full backup\n");
    }

    struct timespec sleep_time;
    sleep_time.tv_sec = state->sample_period_ms / 1000;
    sleep_time.tv_nsec = (state->sample_period_ms % 1000) * 1000000;

    while (running) {
        // Process inotify events
        process_inotify_events(state);

        // Check for commands
        Command cmd;
        if (read_command(&cmd) > 0) {
            char response[MAX_RESPONSE_LEN];
            process_command(&cmd, state, response, sizeof(response));
            send_response(response);
        }

        // Perform sampling if there are changes or periodic timeout
        state->current_sample++;
        ILOG("Sample #%d\n", state->current_sample);

        if (create_incremental_backup(state) != 0) {
            WLOG("Failed to create incremental backup\n");
        }

        // Sleep until next sample
        nanosleep(&sleep_time, NULL);
    }

    cleanup_ipc();
    ILOG("Daemon stopped\n");
    return 0;
}

int run_interactive(MonitorState *state) {
    ILOG("Interactive mode started, monitoring PID %d in directory %s\n",
         state->monitored_pid, state->monitored_dir);

    if (init_inotify(state) != 0) {
        ELOG("Failed to initialize inotify\n");
        return -1;
    }

    // Create first full backup
    if (create_full_backup(state) != 0) {
        WLOG("Failed to create initial full backup\n");
    }

    printf(
    "Interactive mode. Commands:\n"
    "  status - show current status\n"
    "  show_diff - show latest diff\n"
    "  show_k_diffs <k> - show last k diffs\n"
    "  set_pid <pid> - change monitored PID\n"
    "  set_period <ms> - change sample period\n"
    "  quit - exit\n"
    "\nPress Enter to take a sample, or type a command:\n"
    );


    char input[MAX_CMD_LEN] = "";
    struct timespec sleep_time;
    sleep_time.tv_sec   = state->sample_period_ms / 1000;
    sleep_time.tv_nsec  = (state->sample_period_ms % 1000) * 1000000;

    while (running) {
        // Process inotify events
        process_inotify_events(state);

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        if (state->inotify_fd >= 0) {
            FD_SET(state->inotify_fd, &readfds);
        }
        int max_fd = (state->inotify_fd > STDIN_FILENO) ? state->inotify_fd : STDIN_FILENO;
        timeout.tv_sec = sleep_time.tv_sec;
        timeout.tv_usec = sleep_time.tv_nsec / 1000;

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(input, sizeof(input), stdin) != NULL) {
                // Remove newline
                size_t len = strlen(input);
                if (len > 0 && input[len - 1] == '\n') {
                    input[len - 1] = '\0';
                }

                if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
                    break;
                }

                // Parse command
                Command cmd;
                if (strncmp(input, "show_diff", 9) == 0) {
                    cmd.type = CMD_SHOW_DIFF;
                } else if (strncmp(input, "show_k_diffs", 12) == 0) {
                    cmd.type = CMD_SHOW_K_DIFFS;
                    sscanf(input, "show_k_diffs %d", &cmd.arg);
                } else if (strncmp(input, "set_pid", 7) == 0) {
                    cmd.type = CMD_SET_PID;
                    sscanf(input, "set_pid %d", &cmd.arg);
                } else if (strncmp(input, "set_period", 10) == 0) {
                    cmd.type = CMD_SET_PERIOD;
                    sscanf(input, "set_period %d", &cmd.arg);
                } else if (strcmp(input, "status") == 0) {
                    cmd.type = CMD_STATUS;
                } else if (strlen(input) == 0) {
                    // Empty input - take sample
                    state->current_sample++;
                    printf("Taking sample #%d...\n", state->current_sample);
                    if (create_incremental_backup(state) != 0) {
                        printf("Failed to create incremental backup\n");
                    }
                    continue;
                } else {
                    printf("Unknown command: %s\n", input);
                    continue;
                }

                char response[MAX_RESPONSE_LEN];
                process_command(&cmd, state, response, sizeof(response));
                printf("%s", response);
                continue;
            }
        } else if (ret == 0) {
            // Timeout - take sample
            state->current_sample++;
            printf("Sample #%d (auto)\n", state->current_sample);
            if (create_incremental_backup(state) != 0) {
                printf("Failed to create incremental backup\n");
            }
        }
    }

    ILOG("Interactive mode stopped\n");
    return 0;
}
