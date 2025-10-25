#ifndef COMMON_H
#define COMMON_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(SMALL)
    #define BUF_SIZE 8192
#elif defined(MEDIUM)
    #define BUF_SIZE 65536
#elif defined(LARGE)
    #define BUF_SIZE 2147483649
#else
    #define BUF_SIZE 65536
#endif

#endif // COMMON_H
