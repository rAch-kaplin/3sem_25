#include "common.h"
#include "shm.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

static size_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        fprintf(stderr, "failed to get file size\n");
        return 0;
    }
    return (size_t)st.st_size;
}

int main() {
    int fd_in = open("input.txt", O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "failed to open input.txt\n");
        return 1;
    }

    size_t file_size = get_file_size("input.txt");
    if (file_size == 0) {
        fprintf(stderr, "failed to get file size or file is empty\n");
        close(fd_in);
        return 1;
    }

    key_t shm_key = ftok("input.txt", 65);
    if (shm_key == -1) {
        fprintf(stderr, "ftok error\n");
        close(fd_in);
        return 1;
    }

    int old_shmid = shmget(shm_key, 0, 0666);
    if (old_shmid != -1) {
        shmctl(old_shmid, IPC_RMID, NULL);
    }

    size_t shm_total_size = sizeof(SharedData) + SHM_SIZE;

    int shmid = shmget(shm_key, shm_total_size, IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "failed to get shm\n");
        close(fd_in);
        return 1;
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        fprintf(stderr, "failed to get shd ptr\n");
        close(fd_in);
        return 1;
    }

    sh_data = (SharedData *)shm_ptr;
    sh_data->buffer = shm_ptr + sizeof(SharedData);
    sh_data->buf_size = SHM_SIZE;
    sh_data->ppid = getpid();
    sh_data->producer_ended = NOT_ENDED;
    sh_data->consumer_ended = NOT_ENDED;
    sh_data->file_size = file_size;
    sh_data->bytes_read = 0;
    sh_data->attempts = 0;
    sh_data->fd_in = fd_in;

    for (int i = 0; i < CHUNKS; i++) {
        sh_data->producer_chunks[i] = sh_data->buffer + i * CHUNK_SIZE;
        sh_data->consumer_chunks[i].chunk = sh_data->buffer + i * CHUNK_SIZE;
        sh_data->consumer_chunks[i].chunk_size = 0;
    }

    struct sigaction sa = {};
    sa.sa_sigaction = producer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIG_PRODUCER, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction SIG_PRODUCER failed\n");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    sa.sa_sigaction = consumer_handler;

    if (sigaction(SIG_CONSUMER, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction SIG_CONSUMER failed\n");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    sa.sa_flags = 0;
    sa.sa_handler = sig_exit_handler;

    if (sigaction(SIG_EXIT, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction SIG_EXIT failed\n");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    struct timespec start = {}, end = {};

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    if (pid == 0) {
        int fd_out = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "failed to open output.txt file\n");
            exit(1);
        }

        sh_data->ppid = getppid();
        sh_data->fd_out = fd_out;

        union sigval sv = {};
        for (int i = 0; i < CHUNKS; i++) {
            sv.sival_int = i;
            if (sigqueue(sh_data->ppid, SIG_PRODUCER, sv) == -1) {
                fprintf(stderr, "sigqueue initial producer failed\n");
                close(fd_out);
                exit(1);
            }
        }

        while (sh_data->producer_ended == NOT_ENDED) {
            pause();
        }

        int exit_code = (sh_data->consumer_ended == CONS_END_SUCC) ? 0 : 1;

        close(fd_out);
        shmdt(shm_ptr);

        exit(exit_code);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &start);

        sh_data->pid = pid;

        while (sh_data->producer_ended == NOT_ENDED &&
               sh_data->consumer_ended == NOT_ENDED) {
            pause();
        }

        int status;
        waitpid(pid, &status, 0);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
        time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

        fprintf(stderr, "Time duration: %.6f seconds\n", time_taken);

        close(fd_in);

        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);

        return 0;
    }
}
