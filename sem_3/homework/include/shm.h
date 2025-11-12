#ifndef SHM_H
#define SHM_H

#include "common.h"
#include <stdbool.h>

#define SHM_SIZE (4096 * 1024)

typedef struct {
    size_t  buf_size;
    char    buffer[SHM_SIZE];
    bool    eof;
} SharedData;

bool sem_wait   (int semid, int id);
bool sem_signal (int semid, int id);

#endif // SHM_H
