#include <stdio.h>

#include "file.h"
#include "duplex_pipe.h"

int main() {
    DuplexPipe *pipe = CreateDuplexPipe(65536);
    if (pipe == NULL) {
        fprintf(stderr, "failed to create DuplexPipe\n");
        return 1;
    }

    Run(pipe);
    DestroyDuplexPipe(pipe);

    return 0;
}
