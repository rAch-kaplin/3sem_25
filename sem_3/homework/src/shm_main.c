#include "common.h"
#include "shm.h"

int main() {
    int fd_in = open("input.txt", O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "failed to open input.txt file\n");
        return 1;
    }

    key_t shm_key = ftok("input.txt", 65);
    key_t sem_key = ftok("input.txt", 64);
    if (shm_key == -1 || sem_key == -1) {
        fprintf(stderr, "ftok error\n");
        close(fd_in);
        return 1;
    }

    int shmid = shmget(shm_key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "failed to get shm\n");
        close(fd_in); 
        return 1;
    }

    SharedData *shd = (SharedData*)shmat(shmid, NULL, 0);
    if (shd == (void*)-1) {
        fprintf(stderr, "failed to get shd ptr\n");
        close(fd_in);
        return 1;
    }

    int semid = semget(sem_key, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        fprintf(stderr, "failed to get sem\n");
        shmdt(shd);
        close(fd_in);
        return 1;
    }

    if (semctl(semid, 0, SETVAL, 1) == -1 || semctl(semid, 1, SETVAL, 0) == -1) {
        fprintf(stderr, "failed to set val sem\n");
        shmdt(shd);
        close(fd_in);
        return 1;
    }


    struct timespec start = {}, end = {};

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        return -1;
    }

    if (pid == 0) {
        int fd_out = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "failed to open output.txt file\n");
            exit(1);
        }

        while (1) {
            if (!sem_wait(semid, 1)) exit(1);

            if (shd->eof) {
                sem_signal(semid, 0);
                break;
            }

            size_t written = 0;
            while (written < shd->buf_size) {
                ssize_t n = write(fd_out, shd->buffer + written, shd->buf_size - written);
                if (n <= 0) {
                    fprintf(stderr, "failed to write data\n");
                    exit(1);
                }
                written += (size_t)n;
            }

            if (!sem_signal(semid, 0)) exit(1);
        }

        close(fd_out);
        shmdt(shd);
        exit(0);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &start);

        ssize_t n;
        while ((n = read(fd_in, shd->buffer, BUF_SIZE)) > 0) {
            if (!sem_wait(semid, 0)) break;
            shd->buf_size = (size_t)n;
            shd->eof = 0;
            if (!sem_signal(semid, 1)) break;
        }

        if (!sem_wait(semid, 0)) {
            fprintf(stderr, "final sem_wait\n");
        } else {
            shd->eof = 1;
            sem_signal(semid, 1);
        }

        close(fd_in);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = 0.0;
        time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
        time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

        printf("Time duration: %lg\n", time_taken);

        shmdt(shd);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, IPC_RMID, 0);
    }

    return 0;
}

bool sem_wait(int semid, int id) {
    struct sembuf sb = {
        .sem_num    = (unsigned short)id,
        .sem_op     = -1,
    };

    return semop(semid, &sb, 1) == 0;
}

bool sem_signal(int semid, int id) {
    struct sembuf sb = {
        .sem_num    = (unsigned short)id,
        .sem_op     = 1,
    };

    return semop(semid, &sb, 1) == 0;
}
