#include <sys/inotify.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE))

int main(int argc, char **argv) {
    int fd = inotify_init();

    int wd = inotify_add_watch(fd, argv[1], IN_CREATE | IN_DELETE | IN_MODIFY);
    if (wd == -1) {
        return 1;
    }

    char buf[BUF_LEN];
    while (1) {
        int len = read(fd, buf, BUF_LEN);
        int i = 0;

        while (i < len) {
            struct inotify_event *ev = (struct inotify_event*)(&buf[i]);

            if (ev->len > 0) {
                if (ev->mask & IN_CREATE) {
                    printf("Create: file %s\n", ev->name);
                } else if (ev->mask & IN_DELETE) {
                        printf("Delete: file %s\n", ev->name);
                } else if (ev->mask & IN_MODIFY) {
                    printf("Modify: file %s\n", ev->name);
                }
            }

            i += EVENT_SIZE + ev->len;
        }

        inotify_rm_watch(fd, wd);
        close(fd);
    }

    return 0;
}
