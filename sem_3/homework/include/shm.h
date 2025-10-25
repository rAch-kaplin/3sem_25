#ifndef SHM_H
#define SHM_H

#include "common.h"
#include <stdbool.h>

typedef struct {
    size_t  buf_size;
    char    buffer[BUF_SIZE];
    char    eof;
} SharedData;

bool sem_wait   (int semid, int id);
bool sem_signal (int semid, int id);

#endif // SHM_H
