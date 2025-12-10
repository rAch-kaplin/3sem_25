#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "udp_discovery.h"
#include "common.h"
#include "log.h"

int discover_servers_udp(struct ServerList *server_list, int timeout_sec) {
    assert(server_list);

    if (!server_list) {
        return -1;
    }

    server_list->count = 0;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        ELOG_("socket failed: %s", strerror(errno));
        return -1;
    }

    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ELOG_("setsockopt SO_BROADCAST failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        ELOG_("setsockopt SO_RCVTIMEO failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sockaddr_in broadcast_addr = {0};

    broadcast_addr.sin_family      = AF_INET;
    broadcast_addr.sin_port        = htons(UDP_DISCOVERY_PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    if (sendto(sockfd, DISCOVERY_REQUEST, strlen(DISCOVERY_REQUEST), 0,
               (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        ELOG_("sendto failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    DLOG_("Sent UDP discovery broadcast");

    char buffer[256] = "";
    struct sockaddr_in from_addr = {0};
    socklen_t from_len = sizeof(from_addr);

    while (server_list->count < MAX_SERVERS) {
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&from_addr, &from_len);

        if (recv_len < 0) {
            break;
        }

        buffer[recv_len] = '\0';

        if (strncmp(buffer, DISCOVERY_RESPONSE, strlen(DISCOVERY_RESPONSE)) == 0) {
            int found = 0;
            for (size_t i = 0; i < server_list->count; i++) {
                if (server_list->servers[i].addr.s_addr == from_addr.sin_addr.s_addr) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                server_list->servers[server_list->count].addr = from_addr.sin_addr;
                server_list->count++;
                DLOG_("Discovered server: %s", inet_ntoa(from_addr.sin_addr));
            }
        }
    }

    close(sockfd);
    DLOG_("Discovery complete. Found %zu servers", server_list->count);
    return (int)server_list->count;
}

int start_udp_discovery_server(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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

    struct sockaddr_in servaddr = {0};

    servaddr.sin_family         = AF_INET;
    servaddr.sin_addr.s_addr    = INADDR_ANY;
    servaddr.sin_port           = htons(UDP_DISCOVERY_PORT);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ELOG_("bind failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    DLOG_("UDP discovery server listening on port %d", UDP_DISCOVERY_PORT);

    char buffer[256] = "";
    struct sockaddr_in cliaddr = {0};
    socklen_t len = sizeof(cliaddr);

    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&cliaddr, &len);

        if (recv_len < 0) {
            ELOG_("recvfrom failed: %s", strerror(errno));
            continue;
        }

        buffer[recv_len] = '\0';

        if (strcmp(buffer, DISCOVERY_REQUEST) == 0) {
            if (sendto(sockfd, DISCOVERY_RESPONSE, strlen(DISCOVERY_RESPONSE), 0,
                      (struct sockaddr*)&cliaddr, len) < 0) {
                ELOG_("sendto failed: %s", strerror(errno));
            } else {
                DLOG_("Sent discovery response to %s", inet_ntoa(cliaddr.sin_addr));
            }
        }
    }

    close(sockfd);
    return 0;
}
