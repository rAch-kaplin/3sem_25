#include "common.h"

#include <magic.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

bool is_test_file(const char *filename) {
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (magic == NULL) {
        fprintf(stderr, "failed to magic open file\n");
        return false;
    }

    if (magic_load(magic, NULL) != 0) {
        fprintf(stderr, "failed magic load file\n");
        magic_close(magic);
        return false;
    }

    const char *mime = magic_file(magic, filename);

    bool is_text = (mime != NULL) && (strstr(mime, "text") != NULL);
    magic_close(magic);

    return is_text;
}

size_t search_files_in_dir(const char *path, char ***files) {
    size_t count = 0;

    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "failed to open dir: %s\n", path);
        return count;
    }

    char fullpath[256] = "";
    struct stat info;
    struct dirent *e;

     while ((e = readdir(dir))) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, e->d_name);

        if (stat(fullpath, &info) != 0) {
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            search_files_in_dir(fullpath, files);
        } else if (S_ISREG(info.st_mode)) {
            if (is_test_file(fullpath)) {
                *files = (char**)realloc(*files, (count + 1) * sizeof(char*));
                if (*files == NULL) {
                    perror("realloc");
                    closedir(dir);
                    return count;
                }

                (*files)[count] = strdup(fullpath);
                if ((*files)[count] == NULL) {
                    perror("strdup");
                    closedir(dir);
                    return count;
                }

                (count)++;
            }
        }
    }

    closedir(dir);

    return count;
}

