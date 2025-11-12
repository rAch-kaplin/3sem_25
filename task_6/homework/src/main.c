#include "common.h"
#include "config.h"
#include "log.h"

#include <stdio.h>

int main(int argc, char **argv) {
    int err = LoggerInit(LOGL_DEBUG, "debug.log", DEFAULT_MODE);
    if (err != 0) {
        fprintf(stderr, "failed to init log file\n");
    }

    Config cfg = get_cmd_args(argc, argv);
    DLOG("Init config: pid = %zu, mode = %d\n", cfg.pid, cfg.mode);

    LoggerDeinit();
    return 0;
}
