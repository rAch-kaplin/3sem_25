#pragma once

#include "common.h"

#define SHM_SIZE (4096 * 1024)
#define CHUNKS 8
#define CHUNK_SIZE (SHM_SIZE / CHUNKS)
#define MAX_ATTEMPTS 100

#define SIG_PRODUCER (SIGRTMIN + 2)
#define SIG_CONSUMER (SIGRTMIN + 1)
#define SIG_EXIT     (SIGRTMIN)

enum EndCode {
    NOT_ENDED     = 0,
    PROD_END_SUCC = 1,
    PROD_END_FAIL = 2,
    CONS_END_SUCC = 4,
    CONS_END_FAIL = 8
};

typedef struct {
    char    *chunk;
    size_t  chunk_size;
} ChunkData;

typedef struct {
    char    *buffer;
    size_t  buf_size;

    char        *producer_chunks[CHUNKS];
    ChunkData   consumer_chunks[CHUNKS];

    pid_t pid;
    pid_t ppid;

    int producer_ended;
    int consumer_ended;

    size_t  file_size;
    size_t  bytes_read;
    int     attempts;
} SharedData;

extern SharedData *sh_data;

void sig_exit_handler(int sig);
void producer_handler(int sig, siginfo_t *info, void *ucontext);
void consumer_handler(int sig, siginfo_t *info, void *ucontext);
