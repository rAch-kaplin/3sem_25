#include "common.h"
#include "backup.h"
#include "file.h"
#include "log.h"

#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

int ensure_backup_dir() {
    struct stat st = {};
    if (stat(BACKUP_DIR, &st) != 0) {
        if (mkdir(BACKUP_DIR, 0755) != 0) {
            ELOG("Failed to create backup directory: %s\n", BACKUP_DIR);
            return -1;
        }
    }
    return 0;
}

int save_diff(const char *old_file, const char *new_file,
    const char *diff_path, time_t timestamp, int sample_num) {

    char cmd[2048]  = "";
    char line[4096] = "";

    FILE *fp    = NULL;
    FILE *out   = NULL;

    // Create diff using system diff command
    snprintf(cmd, sizeof(cmd), "diff -u \"%s\" \"%s\" 2>/dev/null", old_file, new_file);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        ELOG("Failed to run diff command\n");
        return -1;
    }

    out = fopen(diff_path, "w");
    if (out == NULL) {
        ELOG("Failed to open diff file for writing: %s\n", diff_path);
        pclose(fp);
        return -1;
    }

    // Write metadata
    fprintf(out, "# Timestamp: %ld\n", timestamp);
    fprintf(out, "# Sample: %d\n", sample_num);
    fprintf(out, "# Old file: %s\n", old_file);
    fprintf(out, "# New file: %s\n", new_file);
    fprintf(out, "---\n");

    // Write diff content
    while (fgets(line, sizeof(line), fp) != NULL) {
        fputs(line, out);
    }

    pclose(fp);
    fclose(out);

    return 0;
}

int create_full_backup(MonitorState *state) {
    if (ensure_backup_dir() != 0) {
        return -1;
    }

    char **files = NULL;
    size_t file_count = search_files_in_dir(state->monitored_dir, &files);

    if (file_count == 0) {
        DLOG("No text files found for full backup\n");
        if (files) {
            for (size_t i = 0; i < file_count; i++) {
                free(files[i]);
            }
            free(files);
        }
        return 0;
    }

    char backup_path[MAX_PATH_LEN] = "";
    char relative_path[MAX_PATH_LEN] = "";

    for (size_t i = 0; i < file_count; i++) {
        // Skip debug.log files
        const char *file_basename = strrchr(files[i], '/');
        if (file_basename != NULL) {
            file_basename++; // Skip the '/'
        } else {
            file_basename = files[i];
        }
        if (strcmp(file_basename, "debug.log") == 0) {
            DLOG("Skipping debug.log in full backup: %s\n", files[i]);
            continue;
        }

        // Create relative path for backup
        relative_path[0] = '\0';
        if (strncmp(files[i], state->monitored_dir, strlen(state->monitored_dir)) == 0) {
            size_t dir_len = strlen(state->monitored_dir);
            size_t file_len = strlen(files[i]);
            if (file_len > dir_len) {
                strncpy(relative_path, files[i] + dir_len, sizeof(relative_path) - 1);
                relative_path[sizeof(relative_path) - 1] = '\0';
                // Remove leading slash if present
                if (relative_path[0] == '/') {
                    memmove(relative_path, relative_path + 1, strlen(relative_path));
                }
            } else {
                // Fallback: use filename only
                strncpy(relative_path, file_basename, sizeof(relative_path) - 1);
                relative_path[sizeof(relative_path) - 1] = '\0';
            }
        } else {
            // File is not under monitored_dir, use full path (shouldn't happen normally)
            strncpy(relative_path, files[i], sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';
        }

        // Replace / with _ for filename
        for (size_t j = 0; j < strlen(relative_path); j++) {
            if (relative_path[j] == '/') {
                relative_path[j] = '_';
            }
        }


        int wn = snprintf(backup_path, sizeof(backup_path), "%s/full_%s", BACKUP_DIR, relative_path);
        if (wn >= (int)sizeof(backup_path)) {
            ELOG("Backup path too long, truncated! File: %s\n", relative_path);
            continue;
        }

        // Copy file
        FILE *src = fopen(files[i], "r");
        if (src == NULL) {
            WLOG("Failed to open source file for backup: %s\n", files[i]);
            continue;
        }

        FILE *dst = fopen(backup_path, "w");
        if (dst == NULL) {
            WLOG("Failed to open backup file for writing: %s\n", backup_path);
            fclose(src);
            continue;
        }

        char buffer[4096];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, n, dst);
        }

        fclose(src);
        fclose(dst);

        DLOG("Backed up file: %s -> %s (relative: %s)\n", files[i], backup_path, relative_path);
    }

    // Free files array
    for (size_t i = 0; i < file_count; i++) {
        free(files[i]);
    }
    free(files);

    state->first_backup_done = true;
    ILOG("Full backup completed: %zu files in directory %s\n", file_count, state->monitored_dir);

    return 0;
}

int get_changed_files_from_queue(MonitorState *state, char ***changed_files, size_t *count) {
    *count = state->changed_files_count;
    *changed_files = NULL;

    if (state->changed_files_count == 0) {
        return 0;
    }

    *changed_files = (char**)malloc(state->changed_files_count * sizeof(char*));
    if (*changed_files == NULL) {
        return -1;
    }

    for (size_t i = 0; i < state->changed_files_count; i++) {
        (*changed_files)[i] = strdup(state->changed_files_queue[i]);
        if ((*changed_files)[i] == NULL) {
            // Free what we've allocated so far
            for (size_t j = 0; j < i; j++) {
                free((*changed_files)[j]);
            }
            free(*changed_files);
            *changed_files = NULL;
            return -1;
        }
    }

    // Clear the queue
    for (size_t i = 0; i < state->changed_files_count; i++) {
        free(state->changed_files_queue[i]);
    }
    free(state->changed_files_queue);
    state->changed_files_queue = NULL;
    state->changed_files_count = 0;

    return 0;
}

int find_changed_files(MonitorState *state, char ***changed_files, size_t *count) {
    // Use inotify queue if available
    if (state->inotify_fd >= 0 && state->changed_files_count > 0) {
        return get_changed_files_from_queue(state, changed_files, count);
    }

    // Fallback to stat-based detection (for compatibility)
    *count = 0;
    *changed_files = NULL;

    char **current_files = NULL;
    size_t current_count = search_files_in_dir(state->monitored_dir, &current_files);

    // Check each current file against stored state
    for (size_t i = 0; i < current_count; i++) {
        bool found = false;
        struct stat st;

        if (stat(current_files[i], &st) != 0) {
            continue;
        }

        // Find in state
        for (size_t j = 0; j < state->file_count; j++) {
            if (strcmp(state->files[j].path, current_files[i]) == 0) {
                found = true;
                // Check if modified
                if (state->files[j].mtime != st.st_mtime ||
                    state->files[j].size != st.st_size ||
                    !state->files[j].exists) {
                    // File changed
                    *changed_files = (char**)realloc(*changed_files, (*count + 1) * sizeof(char*));
                    if (*changed_files == NULL) {
                        perror("realloc");
                        goto cleanup;
                    }
                    (*changed_files)[*count] = strdup(current_files[i]);
                    (*count)++;
                }
                // Update state
                state->files[j].mtime = st.st_mtime;
                state->files[j].size = st.st_size;
                state->files[j].exists = true;
                break;
            }
        }

        // New file
        if (!found) {
            *changed_files = (char**)realloc(*changed_files, (*count + 1) * sizeof(char*));
            if (*changed_files == NULL) {
                perror("realloc");
                goto cleanup;
            }
            (*changed_files)[*count] = strdup(current_files[i]);
            (*count)++;

            // Add to state
            if (state->file_count >= state->capacity) {
                state->capacity = state->capacity == 0 ? MAX_FILES : state->capacity * 2;
                state->files = (FileInfo*)realloc(state->files, state->capacity * sizeof(FileInfo));
                if (state->files == NULL) {
                    perror("realloc");
                    goto cleanup;
                }
            }
            strncpy(state->files[state->file_count].path, current_files[i], MAX_PATH_LEN - 1);
            state->files[state->file_count].mtime = st.st_mtime;
            state->files[state->file_count].size = st.st_size;
            state->files[state->file_count].exists = true;
            state->file_count++;
        }
    }

cleanup:
    for (size_t i = 0; i < current_count; i++) {
        free(current_files[i]);
    }
    free(current_files);

    return 0;
}

int create_incremental_backup(MonitorState *state) {
    if (ensure_backup_dir() != 0) {
        return -1;
    }

    char **changed_files = NULL;
    size_t changed_count = 0;

    if (find_changed_files(state, &changed_files, &changed_count) != 0) {
        return -1;
    }

    if (changed_count == 0) {
        DLOG("No changes detected in sample %d\n", state->current_sample);
        if (changed_files) {
            for (size_t i = 0; i < changed_count; i++) {
                free(changed_files[i]);
            }
            free(changed_files);
        }
        return 0;
    }

    time_t now = time(NULL);
    char relative_path[MAX_PATH_LEN];
    char old_backup_path[MAX_PATH_LEN];
    char diff_path[MAX_PATH_LEN];

    for (size_t i = 0; i < changed_count; i++) {
        // Skip debug.log files
        const char *file_basename = strrchr(changed_files[i], '/');
        if (file_basename != NULL) {
            file_basename++; // Skip the '/'
        } else {
            file_basename = changed_files[i];
        }
        if (strcmp(file_basename, "debug.log") == 0) {
            DLOG("Skipping debug.log file: %s\n", changed_files[i]);
            continue;
        }

        // Create relative path
        relative_path[0] = '\0';
        if (strncmp(changed_files[i], state->monitored_dir, strlen(state->monitored_dir)) == 0) {
            size_t dir_len = strlen(state->monitored_dir);
            size_t file_len = strlen(changed_files[i]);
            if (file_len > dir_len) {
                strncpy(relative_path, changed_files[i] + dir_len, sizeof(relative_path) - 1);
                relative_path[sizeof(relative_path) - 1] = '\0';
                // Remove leading slash if present
                if (relative_path[0] == '/') {
                    memmove(relative_path, relative_path + 1, strlen(relative_path));
                }
            } else {
                // Fallback: use filename only
                strncpy(relative_path, file_basename, sizeof(relative_path) - 1);
                relative_path[sizeof(relative_path) - 1] = '\0';
            }
        } else {
            // File is not under monitored_dir, use full path (shouldn't happen normally)
            strncpy(relative_path, changed_files[i], sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';
        }

        // Replace / with _ for filename
        for (size_t j = 0; j < strlen(relative_path); j++) {
            if (relative_path[j] == '/') {
                relative_path[j] = '_';
            }
        }

        // Find old backup file
        int wn = snprintf(old_backup_path, sizeof(old_backup_path), "%s/full_%s", BACKUP_DIR, relative_path);
        if (wn >= (int)sizeof(old_backup_path)) {
            ELOG("Old backup path too long, truncated! File: %s\n", relative_path);
            continue;
        }

        struct stat st = {};
        if (stat(old_backup_path, &st) != 0) {
            // No old backup, create full backup for this file
            FILE *src = fopen(changed_files[i], "r");
            if (src != NULL) {
                FILE *dst = fopen(old_backup_path, "w");
                if (dst != NULL) {
                    char buffer[4096];
                    size_t n = 0;
                    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                        fwrite(buffer, 1, n, dst);
                    }
                    fclose(dst);
                }
                fclose(src);
            }
        }

        // Create diff
        int wn_diff = snprintf(diff_path, sizeof(diff_path), "%s/diff_%s_sample_%d", BACKUP_DIR, relative_path, state->current_sample);
        if (wn_diff >= (int)sizeof(old_backup_path)) {
            ELOG("Diff path too long, truncated! File: %s\n", relative_path);
            continue;
        }

        DLOG("Creating diff: %s -> %s (sample %d)\n", changed_files[i], diff_path, state->current_sample);
        save_diff(old_backup_path, changed_files[i], diff_path, now, state->current_sample);

        // Update old backup to new version
        FILE *src = fopen(changed_files[i], "r");
        if (src != NULL) {
            FILE *dst = fopen(old_backup_path, "w");
            if (dst != NULL) {
                char buffer[4096];
                size_t n;
                while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, n, dst);
                }
                fclose(dst);
            }
            fclose(src);
        }

        DLOG("Created diff for file: %s\n", changed_files[i]);
    }

    // Free changed files array
    for (size_t i = 0; i < changed_count; i++) {
        free(changed_files[i]);
    }
    free(changed_files);

    ILOG("Incremental backup completed: %zu files changed in sample %d\n", changed_count, state->current_sample);

    return 0;
}

int get_diff_content(const char *diff_path, char **content, size_t *len) {
    FILE *fp = fopen(diff_path, "r");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(fp);
        return -1;
    }

    *content = (char*)malloc((size_t)file_size + 1);
    if (*content == NULL) {
        fclose(fp);
        return -1;
    }

    size_t read = fread(*content, 1, (size_t)file_size, fp);
    (*content)[read] = '\0';
    *len = read;

    fclose(fp);
    return 0;
}


