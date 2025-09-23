#ifndef DUPLEX_PIPE_H
#define DUPLEX_PIPE_H

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct DuplexPipe   DuplexPipe;
typedef struct op_table     Ops;

typedef struct op_table  {
    size_t (*rcv)(DuplexPipe *self);
    size_t (*snd)(DuplexPipe *self);
} Ops;

typedef struct DuplexPipe {
        char*   data;           // intermediate buffer
        int     fd_direct[2];   // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int     fd_back[2];     // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        Ops     actions;
} DuplexPipe;

DuplexPipe* CreatePipe(size_t buffer_size);
void        DestroyPipe(DuplexPipe *pipe);

#endif // DUPLEX_PIPE_H
