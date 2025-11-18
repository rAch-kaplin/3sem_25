#ifndef DAEMON_H
#define DAEMON_H

#include "common.h"
#include "config.h"

#include <sys/types.h>
#include <sys/inotify.h>
#include <time.h>

#define FIFO_PATH   "/tmp/daemon_monitor_fifo"
#define BACKUP_DIR  "/tmp/daemon_backups"

#define MAX_PATH_LEN                4096
#define MAX_FILES                   1000
#define DEFAULT_SAMPLE_PERIOD_MS    5000

typedef struct {
    char    path[MAX_PATH_LEN];
    time_t  mtime;
    //off_t   size;
    size_t  size;
    bool    exists;
} FileInfo;

// typedef struct {
//     char    file_path[MAX_PATH_LEN];
//     char    diff_path[MAX_PATH_LEN];
//     time_t  timestamp;
//     int     sample_num;
// } BackupEntry;

// Structure to track inotify watch descriptors and their paths
typedef struct {
    int     wd;         // Watch descriptor
    char    *path;      // Full path to the watched directory
} WatchInfo;

typedef struct {

    // A pointer to array of FileInfo elements.
    // Each FileInfo is a record about a specific file,
    // usually containing the path, mtime, size, exists flag, etc.
    FileInfo    *files;

    // The current number of valid FileInfo entries stored in `files`.
    size_t      file_count;

    // The allocated size of the `files` array.
    size_t      capacity;

    // The full path to the directory being monitored by the daemon.
    // All inotify watches and backup operations operate relative to it.
    char        *monitored_dir;

    // The PID of the process being monitored .
    pid_t       monitored_pid;

    // Period between backup cycles, in milliseconds.
    int         sample_period_ms;

    // A counter used to track the current backup interval step.
    int         current_sample;

    // A flag indicating whether the first full backup has already
    // been performed. Used to avoid redundant initial copies.
    bool        first_backup_done;

    // The main inotify file descriptor created by inotify_init().
    // It is used to read filesystem change events and register watches.
    int         inotify_fd;

    // Array of watch descriptors and their corresponding directory paths
    WatchInfo   *watch_list;
    size_t      watch_count;
    size_t      watch_capacity;

    // List of file paths that were modified,
    // created or deleted, as reported by inotify.
    char        **changed_files_queue;

    // Number of paths stored in changed_files_queue.
    // Indicates how many pending file-change events need processing.
    size_t      changed_files_count;
} MonitorState;

void signal_handler(int sig);
void daemonize(void);
char* get_process_cwd(pid_t pid);

int     init_monitor_state     (MonitorState *state, pid_t pid);
void    cleanup_monitor_state  (MonitorState *state);
int     init_inotify           (MonitorState *state);
void    cleanup_inotify        (MonitorState *state);
int     process_inotify_events (MonitorState *state);

int     add_inotify_watch_recursive(MonitorState *state, const char *path);
const char* get_watch_path(MonitorState *state, int wd);

int run_daemon      (MonitorState *state);
int run_interactive (MonitorState *state);

#endif //DAEMON_H


