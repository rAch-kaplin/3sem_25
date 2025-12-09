#include "common.h"
#include "ipc.h"
#include "backup.h"
#include "daemon.h"
#include "log.h"
#include <sys/stat.h>
#include <glob.h>

static int fifo_fd = -1;

int init_ipc(void) {
    struct stat st;
    if (stat(FIFO_PATH, &st) != 0) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            ELOG("Failed to create FIFO: %s\n", FIFO_PATH);
            return -1;
        }
    }

    fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd < 0) {
        ELOG("Failed to open FIFO for reading: %s\n", FIFO_PATH);
        return -1;
    }

    ILOG("IPC FIFO initialized: %s\n", FIFO_PATH);
    return 0;
}

void cleanup_ipc(void) {
    if (fifo_fd >= 0) {
        close(fifo_fd);
        fifo_fd = -1;
    }

    unlink(FIFO_PATH);
}

int read_command(Command *cmd) {
    if (fifo_fd < 0) {
        return -1;
    }

    char buffer[MAX_CMD_LEN] = "";
    ssize_t n = read(fifo_fd, buffer, sizeof(buffer) - 1);

    if (n <= 0) {
        if (errno == EAGAIN) {
            // No data available
            return 0;
        }
        return -1;
    }

    buffer[n] = '\0';

    if (strncmp(buffer, "show_diff", 9) == 0) {
        cmd->type = CMD_SHOW_DIFF;
        cmd->arg = 0;
    } else if (strncmp(buffer, "show_k_diffs", 12) == 0) {
        cmd->type = CMD_SHOW_K_DIFFS;
        sscanf(buffer, "show_k_diffs %d", &cmd->arg);
    } else if (strncmp(buffer, "set_pid", 7) == 0) {
        cmd->type = CMD_SET_PID;
        sscanf(buffer, "set_pid %d", &cmd->arg);
    } else if (strncmp(buffer, "set_period", 10) == 0) {
        cmd->type = CMD_SET_PERIOD;
        sscanf(buffer, "set_period %d", &cmd->arg);
    } else if (strncmp(buffer, "status", 6) == 0) {
        cmd->type = CMD_STATUS;
        cmd->arg = 0;
    } else if (strncmp(buffer, "restore", 7) == 0) {
        cmd->type = CMD_RESTORE;
        char filename[MAX_PATH_LEN] = "";
        int sample = 0;
        if (sscanf(buffer, "restore %s %d", filename, &sample) == 2) {
            strncpy(cmd->filename, filename, MAX_PATH_LEN - 1);
            cmd->filename[MAX_PATH_LEN - 1] = '\0';
            cmd->arg = sample;
        } else {
            cmd->type = CMD_UNKNOWN;
        }
    } else {
        cmd->type = CMD_UNKNOWN;
    }

    return 1;
}

int process_command(Command *cmd, MonitorState *state, char *response, size_t response_len) {
    response[0] = '\0';

    switch (cmd->type) {
        case CMD_SHOW_DIFF: {
            // Find latest diff files
            char pattern[MAX_PATH_LEN];
            snprintf(pattern, sizeof(pattern), "%s/diff_*_sample_%d", BACKUP_DIR, state->current_sample);

            glob_t glob_result;
            if (glob(pattern, GLOB_NOSORT, NULL, &glob_result) == 0) {
                strncat(response, "Latest diffs:\n", response_len - strlen(response) - 1);
                for (size_t i = 0; i < glob_result.gl_pathc && i < 5; i++) {
                    char *content = NULL;
                    size_t len = 0;
                    if (get_diff_content(glob_result.gl_pathv[i], &content, &len) == 0) {
                        strncat(response, glob_result.gl_pathv[i], response_len - strlen(response) - 1);
                        strncat(response, ":\n", response_len - strlen(response) - 1);
                        strncat(response, content, response_len - strlen(response) - 1);
                        strncat(response, "\n---\n", response_len - strlen(response) - 1);
                        free(content);
                    }
                }
                globfree(&glob_result);
            } else {
                strncat(response, "No diffs found for current sample\n", response_len - strlen(response) - 1);
            }
            break;
        }

        case CMD_SHOW_K_DIFFS: {
            int k = cmd->arg;
            if (k <= 0 || k > state->current_sample) {
                k = state->current_sample;
            }

            strncat(response, "Diffs for last ", response_len - strlen(response) - 1);
            char k_str[32];
            snprintf(k_str, sizeof(k_str), "%d", k);
            strncat(response, k_str, response_len - strlen(response) - 1);
            strncat(response, " samples:\n", response_len - strlen(response) - 1);

            for (int sample = state->current_sample - k + 1; sample <= state->current_sample; sample++) {
                if (sample < 0) continue;
                char pattern[MAX_PATH_LEN];
                snprintf(pattern, sizeof(pattern), "%s/diff_*_sample_%d", BACKUP_DIR, sample);

                glob_t glob_result;
                if (glob(pattern, GLOB_NOSORT, NULL, &glob_result) == 0) {
                    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
                        strncat(response, "Sample ", response_len - strlen(response) - 1);
                        char sample_str[32];
                        snprintf(sample_str, sizeof(sample_str), "%d", sample);
                        strncat(response, sample_str, response_len - strlen(response) - 1);
                        strncat(response, ": ", response_len - strlen(response) - 1);
                        strncat(response, glob_result.gl_pathv[i], response_len - strlen(response) - 1);
                        strncat(response, "\n", response_len - strlen(response) - 1);
                    }
                    globfree(&glob_result);
                }
            }
            break;
        }

        case CMD_SET_PID: {
            pid_t new_pid = (pid_t)cmd->arg;
            char *new_cwd = get_process_cwd(new_pid);
            if (new_cwd != NULL) {
                free(state->monitored_dir);
                state->monitored_dir = strdup(new_cwd);
                state->monitored_pid = new_pid;
                state->first_backup_done = false;
                state->file_count = 0;
                snprintf(response, response_len, "PID changed to %d, monitoring directory: %s\n", new_pid, new_cwd);
            } else {
                snprintf(response, response_len, "Failed to get CWD for PID %d\n", new_pid);
            }
            break;
        }

        case CMD_SET_PERIOD: {
            int new_period = cmd->arg;
            if (new_period > 0 && new_period < 3600000) { // Max 1 hour
                state->sample_period_ms = new_period;
                snprintf(response, response_len, "Sample period changed to %d ms\n", new_period);
            } else {
                snprintf(response, response_len, "Invalid period: %d (must be 1-3600000 ms)\n", new_period);
            }
            break;
        }

        case CMD_STATUS: {
            snprintf(response, response_len,
                    "Status:\n"
                    "  Monitored PID: %d\n"
                    "  Directory: %s\n"
                    "  Sample period: %d ms\n"
                    "  Current sample: %d\n"
                    "  Files tracked: %zu\n"
                    "  First backup done: %s\n",
                    state->monitored_pid,
                    state->monitored_dir ? state->monitored_dir : "N/A",
                    state->sample_period_ms,
                    state->current_sample,
                    state->file_count,
                    state->first_backup_done ? "yes" : "no");
            break;
        }

        case CMD_RESTORE: {
            int result = restore_file_to_sample(state, cmd->filename, cmd->arg);
            if (result == 0) {
                const char *file_basename = strrchr(cmd->filename, '/');
                if (file_basename != NULL) {
                    file_basename++;
                } else {
                    file_basename = cmd->filename;
                }

                char restored_filename[MAX_PATH_LEN] = "";
                const char *dot = strrchr(file_basename, '.');
                if (dot != NULL) {
                    size_t name_len = dot - file_basename;
                    snprintf(restored_filename, sizeof(restored_filename), "%.*s_restored_%d%s",
                             (int)name_len, file_basename, cmd->arg, dot);
                } else {
                    snprintf(restored_filename, sizeof(restored_filename), "%s_restored_%d",
                             file_basename, cmd->arg);
                }

                snprintf(response, response_len,
                        "File %s restored to sample %d as %s\n", cmd->filename, cmd->arg, restored_filename);
            } else {
                snprintf(response, response_len,
                        "Failed to restore file %s to sample %d\n", cmd->filename, cmd->arg);
            }
            break;
        }

        case CMD_UNKNOWN:
        default:
            snprintf(response, response_len, "Unknown command\n");
            break;
    }

    return 0;
}

int send_response(const char *response) {
    // For daemon mode, responses go to log
    // For interactive mode, responses go to stdout
    ILOG("Response: %s\n", response);
    return 0;
}

