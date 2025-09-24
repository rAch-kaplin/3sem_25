#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <duplex_pipe.h>

const size_t MAX_BUF_SIZE = 1 << 16; // 65536

static ssize_t ReadDuplex(DuplexPipe *pipe) {
    assert(pipe);

    return read(pipe->fd_direct[0], pipe->data, MAX_BUF_SIZE);
}

static ssize_t WriteDuplex(DuplexPipe *pipe) {
    assert(pipe);

    return write(pipe->fd_back[1], pipe->data, pipe->len);
}

void Run(DuplexPipe *self) {
    assert(self);

    pid_t pid = -1;

    if ((pid = fork()) == -1) {
        fprintf(stderr, "failed to create new process\n");
        return;
    } else if (pid == 0) {
        close(self->fd_direct[1]);   // no write in parent -> child
        close(self->fd_back[0]);     // no read from child -> parent

        ssize_t n;
        while ((n = self->actions.rcv(self)) > 0) {
            self->len = n;
            self->actions.snd(self);
        }

        exit(0);
    } else {
        close(self->fd_direct[0]);
        close(self->fd_back[1]);

        int in  = open("parent.txt", O_RDONLY);
        if (in == -1) {
            fprintf(stderr, "failed to open parent file\n");
            return;
        }

        int out = open("child.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1) {
            fprintf(stderr, "failed to open child file\n");
            close(in);
            return;
        }

        ssize_t len = 0;
        while ((len = read(in, self->data, MAX_BUF_SIZE)) > 0) {
            self->len = len;
            if (self->actions.snd(self) != len) break;
            if (self->actions.rcv(self) != len) break;
            write(out, self->data, len);
        }

        close(in);
        close(out);

        wait(NULL);
    }
}

DuplexPipe* CreateDuplexPipe(size_t buffer_size) {
    DuplexPipe* self = (DuplexPipe*)calloc(1, sizeof(DuplexPipe));
    if (self == NULL) {
        fprintf(stderr, "failed to allocate memory for Pipe\n");
        return NULL;
    }

    self->data = (char*)calloc(buffer_size, sizeof(char));
    if (self->data == NULL) {
        fprintf(stderr, "failed to allocate memory for data");
        return NULL;
    }

    if (pipe(self->fd_direct) == -1 || pipe(self->fd_back) == -1) {
        fprintf(stderr, "failed to initialize pipe\n");
        free(self->data);
        free(self);

        return NULL;
    }

    self->actions.rcv = ReadDuplex;
    self->actions.snd = WriteDuplex;

    return self;
}

void DestroyDuplexPipe(DuplexPipe *self) {
    if (self == NULL) return;

    close(self->fd_direct[0]);
    close(self->fd_direct[1]);

    close(self->fd_back[0]);
    close(self->fd_back[1]);

    free(self->data);
    free(self);
}

