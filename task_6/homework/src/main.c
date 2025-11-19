#include "common.h"
#include "config.h"
#include "daemon.h"
#include "log.h"

#include <signal.h>


int main(int argc, char **argv) {

    int err = LoggerInit(LOGL_DEBUG, "debug.log", DEFAULT_MODE);
    if (err != 0) {
        fprintf(stderr, "Failed to init log file\n");
        return EXIT_FAILURE;
    }

    Config cfg = get_cmd_args(argc, argv);
    DLOG("Init config: pid = %d, mode = %d\n", (int)cfg.pid, cfg.mode);

    MonitorState *monitor_state = (MonitorState*)calloc(1, sizeof(MonitorState));
    if (monitor_state == NULL) {
        ELOG("failed to allocated memory");
        return 1;
    }

    if (init_monitor_state(monitor_state, cfg.pid) != 0) {
        ELOG("Failed to initialize monitor state\n");
        LoggerDeinit();
        return 1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    int result = 0;

    if (cfg.mode == DAEMON) {

        daemonize();

        result = run_daemon(monitor_state);
    } else {

        result = run_interactive(monitor_state);
    }

    if (result != 0) {
        fprintf(stderr, "The program did not work correctly, see the logs\n");
    }

    cleanup_monitor_state(monitor_state);
    LoggerDeinit();

    return 0;
}
