#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT        8125
#define BUF_SIZE    4096

void* handler(void *argv) {
    int connfd = *((int*)argv);
    free(argv);  

    char buf[BUF_SIZE];

    while (1) {
        int n = read(connfd, buf, BUF_SIZE - 1);
        if (n <= 0) break; 
        buf[n] = '\0';

        printf("From %d: %s\n", connfd, buf);

        if (write(connfd, buf, n) < 0) {
            perror("write");
            break;
        }
    }

    printf("Client %d disconnected\n", connfd);
    close(connfd);
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr = {0}, cliaddr = {0};

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(sockfd, 5) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        socklen_t len = sizeof(cliaddr);
        int *connfd = malloc(sizeof(int));
        if (!connfd) {
            perror("malloc");
            continue;
        }

        *connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &len);
        if (*connfd < 0) {
            perror("accept");
            free(connfd);
            continue;
        }

        printf("New connection from %s:%d, socket %d\n",
               inet_ntoa(cliaddr.sin_addr),
               ntohs(cliaddr.sin_port),
               *connfd);

        pthread_t tid;
        pthread_create(&tid, NULL, handler, connfd);
        pthread_detach(tid); 
    }

    close(sockfd);
    return 0;
}

