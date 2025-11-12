#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

enum Mode {
    INTERACTIVE = 0,
    DAEMON      = 1,
};

typedef struct {
    enum Mode   mode;
    pid_t       pid;
} Config;

Config get_cmd_args(int argc, char **argv);

#endif //CONFIG_H
