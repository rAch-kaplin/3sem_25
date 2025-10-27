#include "common.h"

#define MQ_SIZE (4096)

struct MQData {
    long mtype;
    char mtext[MQ_SIZE];
};

static const char EOF_MSG[] = "EOF";

int main() {
    int fd_in = open("input.txt", O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "failed to open input.txt\n");
        return 1;
    }

    key_t shm_key = ftok("input.txt", 65);
    if (shm_key == -1) {
        fprintf(stderr, "ftok failed for shared memory\n");
        close(fd_in);
        return 1;
    }

    int shmid = shmget(shm_key, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "failed to create shared memory\n");
        close(fd_in);
        return 1;
    }

    int *eof_flag = (int *)shmat(shmid, NULL, 0);
    if (eof_flag == (void *)-1) {
        fprintf(stderr, "failed to attach shared memory\n");
        close(fd_in);
        return 1;
    }
    *eof_flag = 0;

    key_t mq_key =ftok("output.txt", 64);
    if (mq_key == -1) {
        fprintf(stderr, "ftok failed for message queue\n");
        shmdt(eof_flag);
        shmctl(shmid, IPC_RMID, 0);
        close(fd_in);
        return 1;
    }

    int mqid = msgget(mq_key, IPC_CREAT | 0660);
    if (mqid == -1) {
        fprintf(stderr, "failed to create message queue\n");
        shmdt(eof_flag);
        shmctl(shmid, IPC_RMID, 0);
        close(fd_in);
        return 1;
    }

    struct MQData msg = { .mtype = 1 };
    struct timespec start = {}, end = {};

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        shmdt(eof_flag);
        shmctl(shmid, IPC_RMID, 0);
        msgctl(mqid, IPC_RMID, 0);
        close(fd_in);
        return 1;
    }

    if (pid == 0) {
        int fd_out = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "failed to open output.txt\n");
            exit(1);
        }

        while (1) {
            ssize_t n = msgrcv(mqid, &msg, MQ_SIZE, 0, 0);
            if (n == -1) {
                fprintf(stderr, "msgrcv failed\n");
                close(fd_out);
                exit(1);
            }

            if (*eof_flag && memcmp(msg.mtext, EOF_MSG, sizeof(EOF_MSG) - 1) == 0) {
                break;
            }

            size_t written = 0;
            while (written < (size_t)n) {
                ssize_t w = write(fd_out, msg.mtext + written, n - written);
                if (w <= 0) {
                    fprintf(stderr, "failed to write to output.txt\n");
                    close(fd_out);
                    exit(1);
                }
                written += (size_t)w;
            }
        }

        close(fd_out);
        shmdt(eof_flag);
        exit(0);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &start);

        ssize_t n;
        while ((n = read(fd_in, msg.mtext, MQ_SIZE)) > 0) {
            if (msgsnd(mqid, &msg, n, 0) == -1) {
                fprintf(stderr, "msgsnd failed\n");
                break;
            }
        }

        if (n == -1) {
            fprintf(stderr, "error reading input.txt\n");
            close(fd_in);
            return 1;
        }

        *eof_flag = 1;
        memcpy(msg.mtext, EOF_MSG, sizeof(EOF_MSG) - 1);
        if (msgsnd(mqid, &msg, sizeof(EOF_MSG) - 1, 0) == -1) {
            fprintf(stderr, "failed to send EOF message\n");
        }

        close(fd_in);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = 0.0;
        time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
        time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

        printf("Time duration: %lg\n", time_taken);

        shmdt(eof_flag);
        shmctl(shmid, IPC_RMID, 0);
        msgctl(mqid, IPC_RMID, 0);
    }

    return 0;
}
