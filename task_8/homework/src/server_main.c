#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "udp_discovery.h"
#include "tcp_server.h"
#include "log.h"

static void* udp_server_thread(void *arg) {
    (void)arg;
    start_udp_discovery_server();
    return NULL;
}

static void* tcp_server_thread(void *arg) {
    (void)arg;
    start_tcp_task_server();
    return NULL;
}

int main(void) {
    if (LoggerInit(LOGL_DEBUG, "server.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    DLOG_("Starting Monte Carlo computation server");
    DLOG_("UDP discovery port: %d", UDP_DISCOVERY_PORT);
    DLOG_("TCP task port: %d", TCP_TASK_PORT);

    pthread_t udp_tid, tcp_tid;

    if (pthread_create(&udp_tid, NULL, udp_server_thread, NULL) != 0) {
        ELOG_("pthread_create UDP failed: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    if (pthread_create(&tcp_tid, NULL, tcp_server_thread, NULL) != 0) {
        ELOG_("pthread_create TCP failed: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    pthread_join(udp_tid, NULL);
    pthread_join(tcp_tid, NULL);

    LoggerDeinit();
    return 0;
}
