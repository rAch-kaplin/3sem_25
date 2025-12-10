#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp_client.h"
#include "common.h"

#define BUFFER_SIZE 4096

int send_task_to_server(const struct ServerInfo *server, const struct Task *task, struct Result *result) {
    if (!server || !task || !result) {
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_TASK_PORT);
    servaddr.sin_addr = server->addr;

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int task_len = serialize_task(task, buffer, sizeof(buffer));
    if (task_len < 0) {
        fprintf(stderr, "Failed to serialize task\n");
        close(sockfd);
        return -1;
    }

    if (write(sockfd, buffer, task_len + 1) < 0) {
        perror("write");
        close(sockfd);
        return -1;
    }

    ssize_t recv_len = read(sockfd, buffer, sizeof(buffer) - 1);
    if (recv_len < 0) {
        perror("read");
        close(sockfd);
        return -1;
    }

    buffer[recv_len] = '\0';

    if (deserialize_result(buffer, result) < 0) {
        fprintf(stderr, "Failed to deserialize result\n");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}
