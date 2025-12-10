#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "udp_discovery.h"
#include "tcp_server.h"

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
    printf("Starting Monte Carlo computation server...\n");
    printf("UDP discovery port: %d\n", UDP_DISCOVERY_PORT);
    printf("TCP task port: %d\n", TCP_TASK_PORT);

    pthread_t udp_tid, tcp_tid;

    if (pthread_create(&udp_tid, NULL, udp_server_thread, NULL) != 0) {
        perror("pthread_create UDP");
        return 1;
    }

    if (pthread_create(&tcp_tid, NULL, tcp_server_thread, NULL) != 0) {
        perror("pthread_create TCP");
        return 1;
    }

    pthread_join(udp_tid, NULL);
    pthread_join(tcp_tid, NULL);

    return 0;
}
