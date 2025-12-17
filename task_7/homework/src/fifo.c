#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include "log.h"

int create_fifo_with_path(const char *fifo_path) {
    char path_copy[256] = "";
    strncpy(path_copy, fifo_path, sizeof(path_copy));

    char *last_slash = strrchr(path_copy, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';

        char *current = path_copy;
        while (*current == '/') current++;

        while (1) {
            char *next_slash = strchr(current, '/');
            if (next_slash != NULL) {
                *next_slash = '\0';
            }

            mkdir(path_copy, 0755);

            if (next_slash == NULL) break;

            *next_slash = '/';
            current = next_slash + 1;
        }
    }

    if (mkfifo(fifo_path, 0666) == -1) {
        if (errno != EEXIST) {
            ELOG("mkfifo failed for %s: %s", fifo_path, strerror(errno));
            return -1;
        }
        ILOG("FIFO already exists: %s", fifo_path);
    } else {
        ILOG("Created FIFO: %s", fifo_path);
    }

    return 0;
}

int file_exists(const char *path) {
    if (!path) {
        ELOG("file_exists: null path");
        return 0;
    }

    int result = access(path, F_OK);
    if (result == -1 && errno != ENOENT) {
        ELOG("access() failed for %s: %s", path, strerror(errno));
    }

    return result != -1;
}

void cleanup_fifo(const char *fifo_path) {
    if (!fifo_path) {
        ELOG("cleanup_fifo: null path");
        return;
    }

    if (unlink(fifo_path) == -1) {
        if (errno != ENOENT) {
            ELOG("Failed to unlink FIFO %s: %s", fifo_path, strerror(errno));
        }
    } else {
        ILOG("Removed FIFO: %s", fifo_path);
    }

    char path_copy[256] = "";
    strncpy(path_copy, fifo_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    while (1) {
        char *last_slash = strrchr(path_copy, '/');
        if (last_slash == NULL) break;

        *last_slash = '\0';

        DIR *dir = opendir(path_copy);
        if (dir == NULL) break;

        int is_empty = 1;
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                is_empty = 0;
                break;
            }
        }

        closedir(dir);

        if (!is_empty) break;

        if (rmdir(path_copy) == -1) {
            if (errno != ENOENT && errno != ENOTEMPTY) {
                ELOG("Failed to rmdir %s: %s", path_copy, strerror(errno));
            }
            break;
        }
    }
}
