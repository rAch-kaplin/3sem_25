#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>

#include "log.h"
#include "server.h"
#include "common.h"
#include "fifo.h"


ClientInfo clients[MAX_CLIENTS] = {0};

volatile sig_atomic_t server_running = 1;

int epoll_fd = -1;

void server_signal_handler(int sig) {
    ILOG("Server received signal %d, shutting down...", sig);
    server_running = 0;
}

void setup_server_signals(void) {
    signal(SIGINT, server_signal_handler);
    signal(SIGTERM, server_signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

int find_free_client_slot(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].is_active) {
            return i;
        }
    }
    return -1;
}

void close_client_connection(int client_id) {
    if (client_id < 0 || client_id >= MAX_CLIENTS || !clients[client_id].is_active) {
        WLOG("Attempt to close invalid client connection: %d", client_id);
        return;
    }

    ILOG("Closing connection with client %d", client_id);

    if (epoll_fd != -1 && clients[client_id].rx_fd != -1) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clients[client_id].rx_fd, NULL) == -1) {
            ELOG("Failed to remove client %d from epoll: %s", client_id, strerror(errno));
        }
    }

    if (clients[client_id].tx_fd != -1) {
        if (close(clients[client_id].tx_fd) == -1) {
            ELOG("Failed to close TX FD for client %d: %s", client_id, strerror(errno));
        }
    }

    if (clients[client_id].rx_fd != -1) {
        if (close(clients[client_id].rx_fd) == -1) {
            ELOG("Failed to close RX FD for client %d: %s", client_id, strerror(errno));
        }
    }

    memset(&clients[client_id], 0, sizeof(ClientInfo));

    clients[client_id].client_id    = -1;
    clients[client_id].tx_fd        = -1;
    clients[client_id].rx_fd        = -1;
}

void* send_file_to_client(void *arg) {
    FileTask *task = (FileTask*)arg;

    if (!task) {
        ELOG("send_file_to_client: invalid task argument");
        return NULL;
    }

    ILOG("[Thread] Starting file transfer: %s", task->filename);

    FILE *file = fopen(task->filename, "rb");
    if (!file) {
        ELOG("[Thread] Failed to open file %s: %s", task->filename, strerror(errno));
        free(task);

        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        ELOG("[Thread] Failed to seek to end of file: %s", strerror(errno));
        fclose(file);
        free(task);

        return NULL;
    }

    ssize_t file_size = ftell(file);
    if (file_size < 0) {
        ELOG("[Thread] Failed to get file size: %s", strerror(errno));
        fclose(file);
        free(task);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        ELOG("[Thread] Failed to seek to beginning of file: %s", strerror(errno));
        fclose(file);
        free(task);
        return NULL;
    }

    char size_header[64] = "";
    int ret = snprintf(size_header, sizeof(size_header), "SIZE:%ld\n", file_size);
    if (ret < 0 || ret >= (int)sizeof(size_header)) {
        ELOG("[Thread] Failed to format size header");
        fclose(file);
        free(task);
        return NULL;
    }

    ILOG("[Thread] Sending size header: %s", size_header);
    ssize_t written = write(task->client_fd, size_header, strlen(size_header));
    if (written <= 0) {
        ELOG("[Thread] Failed to send size header: %s", strerror(errno));
        fclose(file);
        free(task);
        return NULL;
    }

    char buffer[BUFFER_SIZE] = "";
    size_t total_sent = 0;

    while (total_sent < (size_t)file_size && server_running) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) {
            if (ferror(file)) {
                ELOG("[Thread] Error reading file: %s", strerror(errno));
            }

            break;
        }

        ssize_t bytes_written = write(task->client_fd, buffer, bytes_read);
        if (bytes_written <= 0) {

            ELOG("[Thread] Error writing to client FIFO: %s", strerror(errno));
            break;
        }

        total_sent += (size_t)bytes_written;
        ILOG("[Thread] Sent: %zu/%ld bytes", total_sent, file_size);
    }

    fclose(file);

    if (total_sent == (size_t)file_size) {
        ILOG("[Thread] Completed file transfer: %s (%zu bytes)", task->filename, total_sent);
    } else {
        WLOG("[Thread] Incomplete file transfer: %s (%zu/%ld bytes)",
             task->filename, total_sent, file_size);
    }

    free(task);
    return NULL;
}

void process_client_command(int client_id, const char *command) {
    if (client_id < 0 || client_id >= MAX_CLIENTS || !clients[client_id].is_active) {
        ELOG("process_client_command: invalid client_id: %d", client_id);
        return;
    }

    if (!command) {
        ELOG("process_client_command: null command");
        return;
    }

    printf("Client %d: %s\n", client_id, command);

    char cmd_copy[512]= "";
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    char *newline = strchr(cmd_copy, '\n');
    if (newline) *newline = '\0';

    if (strncmp(cmd_copy, CMD_DISCONNECT, strlen(CMD_DISCONNECT)) == 0) {
        ILOG("Client %d disconnecting", client_id);
        close_client_connection(client_id);

        return;
    }

    if (strncmp(cmd_copy, CMD_GET, strlen(CMD_GET)) == 0) {
        char filename[256];
        if (sscanf(cmd_copy + strlen(CMD_GET) + 1, "%255s", filename) != 1) {
            ELOG("Invalid GET command format: %s", cmd_copy);

            return;
        }

        if (!file_exists(filename)) {
            ELOG("File does not exist: %s (requested by client %d)", filename, client_id);

            return;
        }

        ILOG("Client %d requested file: %s", client_id, filename);

        FileTask *task = malloc(sizeof(FileTask));
        if (!task) {
            ELOG("Failed to allocate memory for file task: %s", strerror(errno));

            return;
        }

        task->client_fd = clients[client_id].tx_fd;

        strncpy(task->filename, filename, sizeof(task->filename) - 1);

        task->filename[sizeof(task->filename) - 1] = '\0';

        pthread_t thread = {0};
        if (pthread_create(&thread, NULL, send_file_to_client, task) != 0) {
            ELOG("Failed to create thread for file transfer: %s", strerror(errno));
            free(task);
            return;
        }

        pthread_detach(thread);
    } else {
        WLOG("Unknown command from client %d: %s", client_id, cmd_copy);
    }
}

int register_new_client(const char *tx_path, const char *rx_path) {
    if (!tx_path || !rx_path) {
        ELOG("register_new_client: invalid arguments");
        return -1;
    }

    int client_id = find_free_client_slot();
    if (client_id == -1) {
        ELOG("Maximum number of clients reached: %d", MAX_CLIENTS);
        return -1;
    }

    printf("Client %d registered (TX: %s, RX: %s)\n", client_id, tx_path, rx_path);

    int tx_fd = open(tx_path, O_WRONLY | O_NONBLOCK);
    if (tx_fd == -1) {
        ELOG("Failed to open client TX FIFO %s: %s", tx_path, strerror(errno));
        return -1;
    }

    int rx_fd = open(rx_path, O_RDONLY | O_NONBLOCK);
    if (rx_fd == -1) {
        ELOG("Failed to open client RX FIFO %s: %s", rx_path, strerror(errno));
        close(tx_fd);
        return -1;
    }

    clients[client_id].client_id    = client_id;
    clients[client_id].tx_fd        = tx_fd;
    clients[client_id].rx_fd        = rx_fd;

    strncpy(clients[client_id].tx_path, tx_path, sizeof(clients[client_id].tx_path) - 1);
    clients[client_id].tx_path[sizeof(clients[client_id].tx_path) - 1] = '\0';

    strncpy(clients[client_id].rx_path, rx_path, sizeof(clients[client_id].rx_path) - 1);
    clients[client_id].rx_path[sizeof(clients[client_id].rx_path) - 1] = '\0';

    clients[client_id].is_active = 1;

    struct epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.u32 = (uint32_t)client_id;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, rx_fd, &ev) == -1) {
        ELOG("Failed to add client %d to epoll: %s", client_id, strerror(errno));
        close_client_connection(client_id);
        return -1;
    }

    ssize_t written = write(tx_fd, RESP_ACK, strlen(RESP_ACK));
    if (written <= 0) {
        ELOG("Failed to send ACK to client %d: %s", client_id, strerror(errno));
        close_client_connection(client_id);
        return -1;
    }

    char check_buffer[1024] = "";
    ssize_t check_read = read(rx_fd, check_buffer, sizeof(check_buffer) - 1);
    if (check_read > 0) {
        check_buffer[check_read] = '\0';
        ILOG("Found pending command from client %d: %s", client_id, check_buffer);

        char *line = check_buffer;
        char *next_line = NULL;

        while ((next_line = strchr(line, '\n')) != NULL) {
            *next_line = '\0';
            if (strlen(line) > 0) {
                process_client_command(client_id, line);
            }

            line = next_line + 1;
        }
        if (line < check_buffer + check_read && strlen(line) > 0) {
            process_client_command(client_id, line);
        }
    } else if (check_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        WLOG("Error checking for pending commands from client %d: %s", client_id, strerror(errno));
    }

    return 0;
}

void process_registration_commands(int server_fifo_fd) {
    if (server_fifo_fd < 0) {
        ELOG("process_registration_commands: invalid file descriptor");
        return;
    }

    char buffer[1024] = "";

    while (1) {
        ssize_t n = read(server_fifo_fd, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            if (n == -1 && errno == EAGAIN) {
                break;
            }

            if (n == -1) {
                ELOG("Error reading from server FIFO: %s", strerror(errno));
            }

            break;
        }

        buffer[n] = '\0';
        ILOG("Received registration data: %s", buffer);

        char *line = buffer;
        char *next_line = NULL;

        while ((next_line = strchr(line, '\n')) != NULL) {
            *next_line = '\0';

            char tx_path[256] = "", rx_path[256] = "";

            if (sscanf(line, "%*s %255s %255s", tx_path, rx_path) == 2) {
                if (register_new_client(tx_path, rx_path) != 0) {
                    ELOG("Failed to register client: %s %s", tx_path, rx_path);
                }
            } else {
                WLOG("Invalid registration command format: %s", line);
            }

            line = next_line + 1;
        }
    }
}

