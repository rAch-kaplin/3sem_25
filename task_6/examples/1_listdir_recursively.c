#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

void SearchDirectory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;
    
    char fullpath[PATH_MAX];
    struct stat info;
    struct dirent *e;
    
    while ((e = readdir(dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, e->d_name);
        
        if (stat(fullpath, &info)) {
            printf("stat error: %s\n", fullpath);
            continue;
        }
        
        if (S_ISDIR(info.st_mode)) {
            printf("directory: %s/\n", fullpath);
            SearchDirectory(fullpath);
        } else if (S_ISREG(info.st_mode)) {
            printf("reg_file: %s\n", fullpath);
        }
    }
    closedir(dir);
}

int main() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) SearchDirectory(cwd);
    return 0;
}
