#include "common.h"

#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE (4096)
#define FIFO_NAME "fifo_pipe"

int main() {
    int fd_in = open("input.txt", O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "failed to open input.txt\n");
        return 1;
    }

    char *buffer = (char *)calloc(BUF_SIZE, sizeof(char));
    if (!buffer) {
        fprintf(stderr, "failed to allocate buffer\n");
        close(fd_in);
        return 1;
    }

    if (mknod(FIFO_NAME, S_IFIFO | 0666, 0) == -1) {
        fprintf(stderr, "failed to mknod\n");
        free(buffer);
        close(fd_in);
        return 1;
    }

    struct timespec start = {}, end = {};
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        free(buffer);
        close(fd_in);
        return 1;
    }

    if (pid == 0) {
        int fd_out = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "failed to open output.txt\n");
            exit(1);
        }

        int fd_fifo = open(FIFO_NAME, O_RDONLY);
        if (fd_fifo == -1) {
            fprintf(stderr, "failed to open FIFO for reading\n");
            close(fd_out);
            exit(1);
        }

        ssize_t n;
        size_t written;
        while ((n = read(fd_fifo, buffer, BUF_SIZE)) > 0) {
            size_t to_write = (size_t)n;
            written = 0;
            while (written < to_write) {
                ssize_t w = write(fd_out, buffer + written, to_write - written);
                if (w <= 0) {
                    fprintf(stderr, "failed to write to output.txt\n");
                    close(fd_out);
                    close(fd_fifo);
                    exit(1);
                }
                written += (size_t)w;
            }
        }

        close(fd_out);
        close(fd_fifo);
        unlink(FIFO_NAME);
        exit(0);
    } else {
        int fd_fifo = open(FIFO_NAME, O_WRONLY);
        if (fd_fifo == -1) {
            fprintf(stderr, "failed to open FIFO for writing\n");
            free(buffer);
            close(fd_in);
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &start);

        ssize_t n;
        while ((n = read(fd_in, buffer, BUF_SIZE)) > 0) {
            size_t to_write = (size_t)n;
            size_t written = 0;
            while (written < to_write) {
                ssize_t w = write(fd_fifo, buffer + written, to_write - written);
                if (w <= 0) {
                    fprintf(stderr, "failed to write to FIFO\n");
                    close(fd_in);
                    close(fd_fifo);
                    free(buffer);
                    return 1;
                }
                written += (size_t)w;
            }
        }

        if (n == -1) {
            fprintf(stderr, "error reading input.txt\n");
            close(fd_in);
            close(fd_fifo);
            free(buffer);
            return 1;
        }

        close(fd_in);
        close(fd_fifo);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = 0.0;
        time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
        time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

        printf("Time duration: %lg\n", time_taken);

        unlink(FIFO_NAME);
        free(buffer);
    }

    return 0;
}
