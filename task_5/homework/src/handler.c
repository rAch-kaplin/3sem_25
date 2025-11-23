#include "shm.h"
#include <string.h>
#include <errno.h>

SharedData *sh_data = NULL;

// указывает компилятору, что параметр может быть неиспользуемым
void sig_exit_handler(int sig __attribute__((unused))) {
    if (sh_data == NULL) {
        return;
    }

    sh_data->consumer_ended = CONS_END_SUCC;

    struct sigaction sa = {};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIG_CONSUMER, &sa, NULL);
}

void producer_handler(int sig __attribute__((unused)), siginfo_t *info, void *ucontext __attribute__((unused))) {
    if (sh_data == NULL || info == NULL) {
        return;
    }

    int chunk_num = info->si_value.sival_int;

    if (chunk_num < 0 || chunk_num >= CHUNKS) {
        fprintf(stderr, "producer_handler: invalid chunk number %d\n", chunk_num);
        sh_data->producer_ended = PROD_END_FAIL;
        return;
    }

    ssize_t curr_size = read(0, sh_data->producer_chunks[chunk_num], CHUNK_SIZE);

    while (curr_size == 0 &&
           sh_data->bytes_read < sh_data->file_size &&
           sh_data->attempts < MAX_ATTEMPTS) {
        curr_size = read(0, sh_data->producer_chunks[chunk_num], CHUNK_SIZE);
        sh_data->attempts++;
    }

    if (curr_size > 0) {
        sh_data->consumer_chunks[chunk_num].chunk_size = (size_t)curr_size;
        sh_data->bytes_read += (size_t)curr_size;

        union sigval sv;
        sv.sival_int = chunk_num;
        if (sigqueue(sh_data->pid, SIG_CONSUMER, sv) == -1) {
            fprintf(stderr, "sigqueue to consumer: %s\n", strerror(errno));
            sh_data->producer_ended = PROD_END_FAIL;
            return;
        }
    } else {
        if (sh_data->bytes_read == sh_data->file_size) {
            sh_data->producer_ended = PROD_END_SUCC;
        } else {
            fprintf(stderr, "producer_handler: read error\n");
            sh_data->producer_ended = PROD_END_FAIL;
        }

        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIG_PRODUCER, &sa, NULL);

        return;
    }
}

void consumer_handler(int sig __attribute__((unused)), siginfo_t *info, void *ucontext __attribute__((unused))) {
    if (sh_data == NULL || info == NULL) {
        return;
    }

    int chunk_num = info->si_value.sival_int;

    if (chunk_num < 0 || chunk_num >= CHUNKS) {
        fprintf(stderr, "consumer_handler: invalid chunk number %d\n", chunk_num);
        sh_data->consumer_ended = CONS_END_FAIL;
        return;
    }

    size_t bytes_to_write = sh_data->consumer_chunks[chunk_num].chunk_size;
    const char *buf_ptr = sh_data->consumer_chunks[chunk_num].chunk;

    size_t bytes_written_total = 0;
    while (bytes_written_total < bytes_to_write) {
        ssize_t n = write(1, buf_ptr + bytes_written_total,
                         bytes_to_write - bytes_written_total);
        if (n == -1) {
            fprintf(stderr, "write: %s\n", strerror(errno));
            sh_data->consumer_ended = CONS_END_FAIL;

            struct sigaction sa;
            sa.sa_handler = SIG_IGN;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIG_CONSUMER, &sa, NULL);
            return;
        }
        bytes_written_total += (size_t)n;
    }

    union sigval sv = {};
    sv.sival_int = chunk_num;
    if (sigqueue(sh_data->ppid, SIG_PRODUCER, sv) == -1) {
        fprintf(stderr, "sigqueue to producer: %s\n", strerror(errno));
        sh_data->consumer_ended = CONS_END_FAIL;
        return;
    }   
}
