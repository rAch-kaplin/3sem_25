#include "fifo.h"
#include "common.h"
#include "server.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

int main(void) {
    if (LoggerInit(LOGL_INFO, "server.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    printf("=== FIFO Server Started ===\n");
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Server FIFO: %s\n", SERVER_FIFO_PATH);
    printf("Press Ctrl+C to stop\n\n");

    setup_server_signals();

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_id = -1;
        clients[i].tx_fd = -1;
        clients[i].rx_fd = -1;
        clients[i].is_active = 0;
    }

    ILOG("Creating server FIFO...");
    if (create_fifo_with_path(SERVER_FIFO_PATH) != 0) {
        ELOG("Failed to create server FIFO: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    int server_fifo_fd = open(SERVER_FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (server_fifo_fd == -1) {
        ELOG("Failed to open server FIFO: %s", strerror(errno));
        cleanup_fifo(SERVER_FIFO_PATH);
        LoggerDeinit();
        return 1;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        ELOG("epoll_create1 failed: %s", strerror(errno));
        close(server_fifo_fd);
        cleanup_fifo(SERVER_FIFO_PATH);
        LoggerDeinit();
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fifo_fd;
    ev.data.u32 = 0;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fifo_fd, &ev) == -1) {
        ELOG("epoll_ctl failed for server FIFO: %s", strerror(errno));
        close(server_fifo_fd);
        close(epoll_fd);
        cleanup_fifo(SERVER_FIFO_PATH);
        LoggerDeinit();
        return 1;
    }

    printf("Server ready. Waiting for clients...\n\n");

    struct epoll_event events[MAX_CLIENTS + 1];

    while (server_running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_CLIENTS + 1, 1000);

        if (nfds == -1) {
            if (errno == EINTR) {

                continue;
            }

            ELOG("epoll_wait failed: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.u32 == 0) {
                process_registration_commands(server_fifo_fd);
            } else {
                int client_id = (int)events[i].data.u32;

                if (client_id < 0 || client_id >= MAX_CLIENTS ||
                    !clients[client_id].is_active) {
                    WLOG("Invalid client_id in event: %d", client_id);
                    continue;
                }

                ILOG("Epoll event for client %d", client_id);

                // Читаем все доступные команды от клиента
                char buffer[1024];
                ssize_t total_read = 0;

                // Читаем все доступные данные (важно для FIFO)
                while (total_read < (ssize_t)(sizeof(buffer) - 1)) {
                    ssize_t n = read(clients[client_id].rx_fd,
                                     buffer + total_read,
                                     sizeof(buffer) - 1 - total_read);

                    if (n > 0) {
                        total_read += n;
                        // Продолжаем читать, пока есть данные
                        continue;
                    } else if (n == 0) {
                        // EOF - клиент закрыл соединение
                        ILOG("Client %d disconnected (EOF)", client_id);
                        close_client_connection(client_id);
                        break;
                    } else {
                        // Ошибка чтения
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // Нет больше данных
                            break;
                        } else if (errno == ECONNRESET || errno == EPIPE) {
                            ILOG("Client %d disconnected", client_id);
                            close_client_connection(client_id);
                            break;
                        } else {
                            ELOG("Read error from client %d: %s", client_id, strerror(errno));
                            break;
                        }
                    }
                }

                // Обрабатываем все прочитанные команды построчно
                if (total_read > 0) {
                    buffer[total_read] = '\0';

                    // Разбиваем на строки и обрабатываем каждую команду
                    char *line = buffer;
                    char *next_line;

                    while ((next_line = strchr(line, '\n')) != NULL) {
                        *next_line = '\0';
                        if (strlen(line) > 0) {
                            process_client_command(client_id, line);
                        }
                        line = next_line + 1;
                    }

                    // Обрабатываем последнюю строку, если нет завершающего \n
                    if (line < buffer + total_read && strlen(line) > 0) {
                        process_client_command(client_id, line);
                    }
                }
            }
        }

        // Периодически проверяем наличие данных от клиентов (на случай, если epoll пропустил событие)
        static int counter = 0;
        if (++counter % 5 == 0) {
            int active_clients = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_active) {
                    active_clients++;
                    // Проверяем, есть ли данные для чтения (неблокирующий режим)
                    char check_buffer[1024];
                    ssize_t check_read = read(clients[i].rx_fd, check_buffer, sizeof(check_buffer) - 1);
                    if (check_read > 0) {
                        check_buffer[check_read] = '\0';
                        ILOG("Found data from client %d in periodic check: %s", i, check_buffer);
                        // Обрабатываем команды построчно
                        char *line = check_buffer;
                        char *next_line;
                        while ((next_line = strchr(line, '\n')) != NULL) {
                            *next_line = '\0';
                            if (strlen(line) > 0) {
                                process_client_command(i, line);
                            }
                            line = next_line + 1;
                        }
                        if (line < check_buffer + check_read && strlen(line) > 0) {
                            process_client_command(i, line);
                        }
                    } else if (check_read == 0) {
                        // EOF - клиент отключился
                        ILOG("Client %d disconnected (EOF in periodic check)", i);
                        close_client_connection(i);
                    }
                }
            }
            ILOG("[Statistics] Active clients: %d/%d", active_clients, MAX_CLIENTS);
        }
    }

    printf("\n=== Shutting down server ===\n");
    printf("Closing client connections...\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active) {
            close_client_connection(i);
        }
    }

    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    close(server_fifo_fd);

    cleanup_fifo(SERVER_FIFO_PATH);

    printf("Server shutdown complete\n");
    LoggerDeinit();

    return 0;
}
