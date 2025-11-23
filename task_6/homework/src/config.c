#include "common.h"
#include "config.h"

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -p <pid> [-d | -i]\n", prog_name);
}

Config get_cmd_args(int argc, char **argv) {
    Config config = {
        .mode = INTERACTIVE,
        .pid = -1,
    };

    int opt = -1;

    while((opt = getopt(argc , argv, "p:di")) != -1) {
        switch (opt) {
            case 'p':
                config.pid = atoi(optarg);
                break;
            case 'd':
                config.mode = DAEMON;
                break;
            case 'i':
                config.mode = INTERACTIVE;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config.pid == -1) {
        fprintf(stderr, "Error: PID is required.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    return config;
}
