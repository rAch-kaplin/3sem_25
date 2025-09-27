#ifndef DUPLEX_PIPE_H
#define DUPLEX_PIPE_H

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct DuplexPipe   DuplexPipe;
typedef struct op_table     Ops;

typedef struct op_table  {
    ssize_t     (*rcv)(DuplexPipe *self);
    ssize_t     (*snd)(DuplexPipe *self);
    void        (*close_child)(DuplexPipe *self);
    void        (*close_parent)(DuplexPipe *self);
    void        (*close_all)(DuplexPipe *self);
} Ops;

typedef struct DuplexPipe {
        char*   data;           // intermediate buffer
        size_t  data_size;
        int     fd_direct[2];   // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int     fd_back[2];     // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        size_t  len;            // data length in intermediate buffer
        Ops     actions;
} DuplexPipe;

DuplexPipe* CreateDuplexPipe(size_t buffer_size);
void        DestroyDuplexPipe(DuplexPipe *pipe);
void		Run(DuplexPipe *self);

#endif // DUPLEX_PIPE_H
