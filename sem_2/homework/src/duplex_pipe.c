#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

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

static void CloseParentPipes(DuplexPipe *self) {
    assert(self);

    close(self->fd_direct[0]);
    close(self->fd_back[1]);
}

static void CloseChildPipes(DuplexPipe *self) {
    assert(self);

    close(self->fd_direct[1]);
    close(self->fd_back[0]);
}

static void CloseAllPipes(DuplexPipe *self) {
    assert(self);

    close(self->fd_direct[0]);
    close(self->fd_direct[1]);
    close(self->fd_back[0]);
    close(self->fd_back[1]);
}

void Run(DuplexPipe *self) {
    assert(self);

    pid_t pid = -1;

	struct timespec start = {}, end = {};
	clock_gettime(CLOCK_MONOTONIC, &start);

    if ((pid = fork()) == -1) {
        fprintf(stderr, "failed to create new process\n");
        return;
    } else if (pid == 0) {
        /*
            no write in parent -> child
            no read from child -> parent
        */
        self->actions.close_child(self);

        ssize_t n;
        while ((n = self->actions.rcv(self)) > 0) {
            self->len = (size_t)n;
            self->actions.snd(self);
        }

		self->actions.close_all(self);

        exit(0);
    } else {
        self->actions.close_parent(self);

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
            self->len = (size_t)len;
            if (self->actions.snd(self) != len) break;
            if (self->actions.rcv(self) != len) break;
            write(out, self->data, (size_t)len);
        }

		self->actions.close_all(self);

        close(in);
        close(out);
    }

	int status = 0;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
		if (exit_code != 0) {
			printf("Program exited with code %d\n", exit_code);
		}
    }

	clock_gettime(CLOCK_MONOTONIC, &end);
	double time_taken = 0.0;
	time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

	printf("Time duration: %lg\n", time_taken);
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
    self->actions.close_child = CloseChildPipes;
    self->actions.close_parent = CloseParentPipes;
    self->actions.close_all = CloseAllPipes;

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

