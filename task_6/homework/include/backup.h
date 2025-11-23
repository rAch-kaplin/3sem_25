#ifndef BACKUP_H
#define BACKUP_H

#include "common.h"
#include "daemon.h"
#include <sys/stat.h>

int create_full_backup(MonitorState *state);
int create_incremental_backup(MonitorState *state);
int save_diff(const char *old_file, const char *new_file,
    const char *diff_path, time_t timestamp, int sample_num);
int apply_diff(const char *file_path, const char *diff_path);
int get_diff_content(const char *diff_path, char **content, size_t *len);
int find_changed_files(MonitorState *state, char ***changed_files, size_t *count);
int get_changed_files_from_queue(MonitorState *state, char ***changed_files, size_t *count);
int ensure_backup_dir(void);

#endif //BACKUP_H


