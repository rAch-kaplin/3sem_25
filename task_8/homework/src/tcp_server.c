#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "tcp_server.h"
#include "common.h"
#include "monte_carlo.h"
#include "log.h"

#define BUFFER_SIZE 4096

static void* handle_client(void *arg) {
    int connfd = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    ssize_t recv_len = read(connfd, buffer, sizeof(buffer) - 1);

    if (recv_len < 0) {
        ELOG_("read failed: %s", strerror(errno));
        close(connfd);
        return NULL;
    }

    buffer[recv_len] = '\0';

    struct Task task = {0};
    if (deserialize_task(buffer, &task) < 0) {
        ELOG_("Failed to deserialize task");
        close(connfd);
        return NULL;
    }

    DLOG_("Received task: [%.3f, %.3f] x [%.3f, %.3f], points: %zu",
           task.x_min, task.x_max, task.y_min, task.y_max, task.num_points);

    size_t points_inside = 0;
    unsigned int seed = (unsigned int)time(NULL);
    double area = (task.x_max - task.x_min) * (task.y_max - task.y_min);

    for (size_t i = 0; i < task.num_points; i++) {
        double x = task.x_min + (double)rand_r(&seed) / RAND_MAX * (task.x_max - task.x_min);
        double y = task.y_min + (double)rand_r(&seed) / RAND_MAX * (task.y_max - task.y_min);

        if (y <= ExponentialFunc(x)) {
            points_inside++;
        }
    }

    double integral = area * (double)points_inside / (double)task.num_points;

    struct Result result = {0};
    result.integral_value = integral;
    result.total_points = task.num_points;
    result.points_inside = points_inside;

    char result_buffer[BUFFER_SIZE];
    int result_len = serialize_result(&result, result_buffer, sizeof(result_buffer));
    if (result_len < 0) {
        ELOG_("Failed to serialize result");
        close(connfd);
        return NULL;
    }

    if (write(connfd, result_buffer, result_len + 1) < 0) {
        ELOG_("write failed: %s", strerror(errno));
    }

    DLOG_("Sent result: %.15lf", result.integral_value);

    close(connfd);
    return NULL;
}

int start_tcp_task_server(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ELOG_("socket failed: %s", strerror(errno));
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ELOG_("setsockopt SO_REUSEADDR failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(TCP_TASK_PORT);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ELOG_("bind failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 10) < 0) {
        ELOG_("listen failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    DLOG_("TCP task server listening on port %d", TCP_TASK_PORT);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        int *connfd = calloc(1, sizeof(*connfd));
        if (!connfd) {
            ELOG_("calloc failed: %s", strerror(errno));
            continue;
        }

        *connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &len);
        if (*connfd < 0) {
            ELOG_("accept failed: %s", strerror(errno));
            free(connfd);
            continue;
        }

        DLOG_("New connection from %s:%d", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, connfd);
        pthread_detach(tid);
    }

    close(sockfd);
    return 0;
}
