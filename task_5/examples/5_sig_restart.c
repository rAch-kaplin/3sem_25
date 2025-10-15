#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void handler(int sig) {
    printf("\nПолучен сигнал %d\n", sig);
}

int main() {
    struct sigaction sa;
    char buffer[100];
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;  // ВКЛЮЧАЕМ автоматический перезапуск -- read() будет автоматически перезапущен после сигнала
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    printf("Введите текст (Ctrl+C НЕ прервет ввод): ");

    ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer)-1);
    
    if (bytes == -1) {
        printf("read() прерван! errno = %d (%s)\n", errno, strerror(errno));
    } else {
        buffer[bytes] = '\0';
        printf("Вы ввели: %s", buffer);
    }
    
    return 0;
}
