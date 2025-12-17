#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>

#include "udp_discovery.h"
#include "tcp_server.h"
#include "log.h"
#include "common.h"

typedef struct {
    uint16_t tcp_port;
} ServerConfig;

static void* udp_server_thread(void *arg) {
    (void)arg;
    start_udp_discovery_server();
    return NULL;
}

static void* tcp_server_thread(void *arg) {
    ServerConfig *cfg = (ServerConfig*)arg;
    start_tcp_task_server(cfg->tcp_port);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (LoggerInit(LOGL_DEBUG, "server.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    uint16_t tcp_port = TCP_TASK_PORT;

    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p': {
                long p = strtol(optarg, NULL, 10);
                if (p <= 0 || p > 65535) {
                    ELOG_("Invalid TCP port: %s", optarg);
                    LoggerDeinit();
                    return 1;
                }
                tcp_port = (uint16_t)p;
                break;
            }
            case 'h':
                printf("Usage: %s [-p tcp_port]\n", argv[0]);
                printf("  -p tcp_port   TCP task port (default: %d)\n", TCP_TASK_PORT);
                return 0;
            default:
                ELOG_("Unknown option");
                LoggerDeinit();
                return 1;
        }
    }

    DLOG_("Starting Monte Carlo computation server");
    DLOG_("UDP discovery port: %d", UDP_DISCOVERY_PORT);
    DLOG_("TCP task port: %d", tcp_port);

    pthread_t udp_tid, tcp_tid;
    ServerConfig cfg = { .tcp_port = tcp_port };

    if (pthread_create(&udp_tid, NULL, udp_server_thread, NULL) != 0) {
        ELOG_("pthread_create UDP failed: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    if (pthread_create(&tcp_tid, NULL, tcp_server_thread, &cfg) != 0) {
        ELOG_("pthread_create TCP failed: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    pthread_join(udp_tid, NULL);
    pthread_join(tcp_tid, NULL);

    LoggerDeinit();
    return 0;
}
