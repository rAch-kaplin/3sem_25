#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <duplex_pipe.h>

const size_t MAX_BUF_SIZE = 1 << 16;

static size_t ReadDuplex(DuplexPipe *pipe) {
    assert(pipe);

    ssize_t size  = 0;
    size_t  total = 0;
    while (size = read(pipe->fd_direct[0], pipe->data, MAX_BUF_SIZE)) {
        if (size == 0) break;
        if (size <  0) {
            fprintf(stderr, "failed to read batch data\n");
            return 0;
        }

        total += (size_t)size;
    }

    return total;
}

static size_t WriteDuplex(DuplexPipe *pipe) {
    assert(pipe);

    ssize_t size = 0;
    while (size = write(pipe->fd_back[1], pipe->data, MAX_BUF_SIZE)) {
        if (size <  0) {
            fprintf(stderr, "failed to write batch data\n");
            return 0;
        }
    }

    return (size_t)size;
}

DuplexPipe* CreatePipe(size_t buffer_size) {
    DuplexPipe* pipe = (DuplexPipe*)calloc(1, sizeof(Pipe));
    if (pipe == NULL) {
        fprintf(stderr, "failed to allocate memory for Pipe\n");
        return NULL;
    }

    pipe->data = (char*)calloc(buffer_size, sizeof(char));
    if (pipe->data == NULL) {
        fprintf(stderr, "failed to allocate memory for data");
        return NULL;
    }

    if (pipe(pipe->fd_direct) == -1) {
        fprintf(stderr, "failed to initialize fd_direct\n");
        free(pipe->data);
        free(pipe);

        return NULL;
    }

    if (pipe(pipe->fd_back) == -1) {
        fprintf(stderr, "failed to initialize fd_back\n");
        close(pipe->fd_direct[0]);
        close(pipe->fd_direct[1]);
        free(pipe->data);
        free(pipe);

        return NULL;
    }

    pipe->actions.rcv = ReadDuplex;
    pipe->actions.snd = WriteDuplex;

    return pipe;
}

void DestroyPipe(DuplexPipe *pipe) {
    if (pipe == NULL) return;

    close(pipe->fd_direct[0]);
    close(pipe->fd_direct[1]);

    close(pipe->fd_back[0]);
    close(pipe->fd_back[1]);

    free(pipe->data);
    free(pipe);
}

