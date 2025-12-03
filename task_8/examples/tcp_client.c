#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT        8125
#define BUF_SIZE    4096

int main() {
    int sockfd = -1;
    char buffer[BUF_SIZE]   = "";
    char addr[256]          = "";

    struct sockaddr_in servaddr = {0}, cliaddr = {0};
    int connfd = -1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "failed to socket\n");
        return 1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        fprintf(stderr, "failed connect\n");
        return 1;
    }

    while (1) {
        scanf("%s", buffer);

        if (!strcmp(buffer, "exit")) {
            break;
        } else {
            write(sockfd, buffer, strlen(buffer) + 1);
        }

        read(sockfd, buffer, BUF_SIZE);
        printf("%s\n", buffer);
    }

    close(sockfd);

    return 0;
}
