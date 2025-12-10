#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "tcp_client.h"
#include "common.h"
#include "log.h"

#define BUFFER_SIZE 4096
#define BUFFER_SIZE 4096
#define TIMEOUT_MS  15000

int send_task_to_server(const struct ServerInfo *server, const struct Task *task, struct Result *result) {
    if (!server || !task || !result) return -1;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ELOG_("socket failed: %s", strerror(errno));
        return -1;
    }

    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

    struct sockaddr_in servaddr = {0};

    servaddr.sin_family         = AF_INET;
    servaddr.sin_port           = htons(TCP_TASK_PORT);
    servaddr.sin_addr.s_addr    = server->addr.s_addr;

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ELOG_("connect to %s failed: %s", inet_ntoa(server->addr), strerror(errno));
        close(sockfd);
        return -1;
    }

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev = {0}, events[1];

    ev.events   = EPOLLOUT;
    ev.data.fd  = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        ELOG_("epoll_ctl failed: %s", strerror(errno));
        close(sockfd);
        close(epoll_fd);
        return -1;
    }

    char buffer[BUFFER_SIZE] = "";
    int task_len = serialize_task(task, buffer, sizeof(buffer));
    if (task_len < 0) {
        ELOG_("Failed to serialize task");
        close(sockfd);
        close(epoll_fd);
        return -1;
    }

    int state = 0; // 0=connect/send, 1=receive
    size_t sent = 0;
    ssize_t n;
    int ret = -1;

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, 1, TIMEOUT_MS);
        if (nfds <= 0) {
            ELOG_("Timeout or epoll error");
            break;
        }

        if (state == 0) {
            n = write(sockfd, buffer + sent, task_len + 1 - sent);
            if (n > 0) {
                sent += (size_t)n;
            }

            if (sent >= (size_t)(task_len + 1)) {
                state     = 1;
                ev.events = EPOLLIN;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev) < 0) {
                    ELOG_("epoll_ctl failed: %s", strerror(errno));
                    close(sockfd);
                    close(epoll_fd);
                    return -1;
                }
            }
        } else {
            n = read(sockfd, buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                break;
            }

            buffer[n] = '\0';
            if (deserialize_result(buffer, result) >= 0) {
                ret = 0;
                break;
            }
        }
    }

    close(epoll_fd);
    close(sockfd);
    return ret;
}
