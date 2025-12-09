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

// Helper function to create relative path for backup filename
static void make_backup_relative_path(const char *file_path, const char *monitored_dir, char *relative_path, size_t rel_path_size) {
    const char *file_basename = strrchr(file_path, '/');
    if (file_basename != NULL) {
        file_basename++; // Skip the '/'
    } else {
        file_basename = file_path;
    }

    relative_path[0] = '\0';
    if (strncmp(file_path, monitored_dir, strlen(monitored_dir)) == 0) {
        size_t dir_len = strlen(monitored_dir);
        size_t file_len = strlen(file_path);
        if (file_len > dir_len) {
            strncpy(relative_path, file_path + dir_len, rel_path_size - 1);
            relative_path[rel_path_size - 1] = '\0';
            // Remove leading slash if present
            if (relative_path[0] == '/') {
                memmove(relative_path, relative_path + 1, strlen(relative_path));
            }
        } else {
            // Fallback: use filename only
            strncpy(relative_path, file_basename, rel_path_size - 1);
            relative_path[rel_path_size - 1] = '\0';
        }
    } else {
        // File is not under monitored_dir, use full path (shouldn't happen normally)
        strncpy(relative_path, file_path, rel_path_size - 1);
        relative_path[rel_path_size - 1] = '\0';
    }

    // Replace / with _ for filename
    for (size_t j = 0; j < strlen(relative_path); j++) {
        if (relative_path[j] == '/') {
            relative_path[j] = '_';
        }
    }
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
    fprintf(out, "# Timestamp: %ld\n"
                 "# Sample: %d\n"
                 "# Old file: %s\n"
                 "# New file: %s\n"
                 "---\n", timestamp, sample_num, old_file, new_file);

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
        make_backup_relative_path(files[i], state->monitored_dir, relative_path, sizeof(relative_path));


        int wn = snprintf(backup_path, sizeof(backup_path), "%s/full_%s", BACKUP_DIR, relative_path);
        if (wn >= (int)sizeof(backup_path)) {
            ELOG("Backup path too long, truncated! File: %s\n", relative_path);
            continue;
        }

        // Copy file
        FILE *src = fopen(files[i], "r");
        if (src == NULL) {
            ELOG("Failed to open source file for backup: %s\n", files[i]);
            continue;
        }

        FILE *dst = fopen(backup_path, "w");
        if (dst == NULL) {
            ELOG("Failed to open backup file for writing: %s\n", backup_path);
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
    //*changed_files = NULL;

    if (state->changed_files_count == 0) {
        return 0;
    }

    *changed_files = (char**)calloc(state->changed_files_count, sizeof(char*));
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
                        ELOG("failed realloc");
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
                ELOG("failed realloc");
                goto cleanup;
            }
            (*changed_files)[*count] = strdup(current_files[i]);
            (*count)++;

            // Add to state
            if (state->file_count >= state->capacity) {
                state->capacity = state->capacity == 0 ? MAX_FILES : state->capacity * 2;
                state->files = (FileInfo*)realloc(state->files, state->capacity * sizeof(FileInfo));
                if (state->files == NULL) {
                    ELOG("failed realloc");
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
        // DLOG("No changes detected in sample %d\n", state->current_sample);
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
        make_backup_relative_path(changed_files[i], state->monitored_dir, relative_path, sizeof(relative_path));

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
        // Создаем diff от базового бэкапа (начальная версия) к текущей версии
        save_diff(old_backup_path, changed_files[i], diff_path, now, state->current_sample);

        DLOG("Created diff for file: %s\n", changed_files[i]);
    }

    for (size_t i = 0; i < changed_count; i++) {
        free(changed_files[i]);
    }
    free(changed_files);

    ILOG("Incremental backup completed: %zu files changed in sample %d\n", changed_count, state->current_sample);

    return (int)changed_count;
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

    *content = (char*)calloc((size_t)file_size + 1, sizeof(char));
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

int restore_file_to_sample(MonitorState *state, const char *filename, int target_sample) {
    if (filename == NULL || strlen(filename) == 0) {
        ELOG("Invalid filename for restore\n");
        return -1;
    }

    if (target_sample < 1 || target_sample > state->current_sample) {
        ELOG("Invalid sample number: %d (current: %d)\n", target_sample, state->current_sample);
        return -1;
    }

    char file_path[MAX_PATH_LEN] = "";
    char relative_path[MAX_PATH_LEN] = "";

    snprintf(file_path, sizeof(file_path), "%s/%s",
             state->monitored_dir, filename);

    make_backup_relative_path(file_path, state->monitored_dir, relative_path, sizeof(relative_path));

    char base_backup_path[MAX_PATH_LEN] = "";
    snprintf(base_backup_path, sizeof(base_backup_path), "%s/full_%s", BACKUP_DIR, relative_path);

    struct stat st;
    if (stat(base_backup_path, &st) != 0) {
        ELOG("Base backup not found: %s\n", base_backup_path);
        return -1;
    }

    char temp_file[MAX_PATH_LEN] = "";
    snprintf(temp_file, sizeof(temp_file), "%s/temp_restore_%s_%d",
             BACKUP_DIR, relative_path, getpid());

    FILE *src = fopen(base_backup_path, "r");
    if (src == NULL) {
        ELOG("Failed to open base backup: %s\n", base_backup_path);
        return -1;
    }

    FILE *dst = fopen(temp_file, "w");
    if (dst == NULL) {
        ELOG("Failed to create temp file: %s\n", temp_file);
        fclose(src);
        return -1;
    }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dst);
    }
    fclose(src);
    fclose(dst);

    char diff_path[MAX_PATH_LEN] = "";
    snprintf(diff_path, sizeof(diff_path), "%s/diff_%s_sample_%d",
             BACKUP_DIR, relative_path, target_sample);

    if (stat(diff_path, &st) == 0) {
        FILE *diff_fp = fopen(diff_path, "r");
        if (diff_fp != NULL) {
            char clean_diff_path[MAX_PATH_LEN] = "";
            snprintf(clean_diff_path, sizeof(clean_diff_path), "%s/temp_clean_diff_%s_%d_%d",
                     BACKUP_DIR, relative_path, target_sample, getpid());

            FILE *clean_diff_fp = fopen(clean_diff_path, "w");
            if (clean_diff_fp != NULL) {
                char line[4096];
                bool past_metadata = false;
                while (fgets(line, sizeof(line), diff_fp) != NULL) {
                    if (line[0] == '#') {
                        continue;
                    }
                    if (strncmp(line, "---\n", 4) == 0) {
                        past_metadata = true;
                        continue;
                    }
                    if (past_metadata || line[0] != '#') {
                        fputs(line, clean_diff_fp);
                    }
                }
                fclose(clean_diff_fp);
            }
            fclose(diff_fp);

            char patch_cmd[2048];
            snprintf(patch_cmd, sizeof(patch_cmd),
                    "patch -u \"%s\" < \"%s\" 2>/dev/null",
                    temp_file, clean_diff_path);

            int patch_result = system(patch_cmd);
            if (patch_result != 0) {
                DLOG("Patch failed for sample %d (exit code: %d)\n", target_sample, patch_result);
                unlink(clean_diff_path);
                unlink(temp_file);
                return -1;
            }

            unlink(clean_diff_path);
        } else {
            ELOG("Failed to open diff file: %s\n", diff_path);
            unlink(temp_file);
            return -1;
        }
    } else {
        DLOG("No diff file for sample %d, using base backup as-is\n", target_sample);
    }

    const char *file_basename = strrchr(filename, '/');
    if (file_basename != NULL) {
        file_basename++; // Skip the '/'
    } else {
        file_basename = filename;
    }

    char restored_filename[MAX_PATH_LEN] = "";
    const char *dot = strrchr(file_basename, '.');
    if (dot != NULL) {
        size_t name_len = dot - file_basename;
        snprintf(restored_filename, sizeof(restored_filename), "%.*s_restored_%d%s",
                 (int)name_len, file_basename, target_sample, dot);
    } else {
        snprintf(restored_filename, sizeof(restored_filename), "%s_restored_%d",
                 file_basename, target_sample);
    }

    char restored_file_path[MAX_PATH_LEN] = "";
    snprintf(restored_file_path, sizeof(restored_file_path), "%s/%s",
             state->monitored_dir, restored_filename);

    src = fopen(temp_file, "r");
    if (src == NULL) {
        ELOG("Failed to open restored temp file\n");
        unlink(temp_file);
        return -1;
    }

    dst = fopen(restored_file_path, "w");
    if (dst == NULL) {
        ELOG("Failed to open restored file for writing: %s\n", restored_file_path);
        fclose(src);
        unlink(temp_file);
        return -1;
    }

    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dst);
    }
    fclose(src);
    fclose(dst);

    unlink(temp_file);

    ILOG("File %s restored to sample %d as %s\n", filename, target_sample, restored_filename);
    return 0;
}
