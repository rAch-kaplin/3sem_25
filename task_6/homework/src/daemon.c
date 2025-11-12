#include "common.h"

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "failed fork process\n");
    } else if(pid > 0) {
        exit(0);
    }

    umask(0);
    setsid();
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

char* get_process_cwd(pid_t pid) {
    static char cwd_path[256] = "";
    char proc_path[256] = "";

    ssize_t len;

    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", pid);
    len = readlink(proc_path, cwd_path, sizeof(cwd_path) - 1);
    if (len != -1) {
        cwd_path[len] = '\0';
        return cwd_path;
    } else {
        fprintf(stderr, "failed to readlink\n");
        return NULL;
    }
}
