#include "common.h"
#include "client.h"
#include "log.h"
#include "fifo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <errno.h>

volatile sig_atomic_t should_exit = 0;

void signal_handler(int sig) {
    ILOG("shutdown: %d", sig);
    should_exit = 1;
}

void setup_signals() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

int register_client(Client *client) {
    int preopen_fd = open(client->tx_fifo, O_RDONLY | O_NONBLOCK);
    if (preopen_fd == -1) {
        ELOG("failed to pre-open client TX FIFO %s: %s",
             client->tx_fifo, strerror(errno));
        return -1;
    }

    int server_fifo_fd = open(SERVER_FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (server_fifo_fd == -1) {
        ELOG("failed to open server fifo fd: %s", strerror(errno));
        return -1;
    }

    char cmd_register[512] = "";
    int ret = snprintf(cmd_register, sizeof(cmd_register), "%s %s %s\n", CMD_REGISTER, client->tx_fifo, client->rx_fifo);
    if (ret < 0 || ret >= (int)sizeof(cmd_register)) {
        ELOG("Failed to format register command: buffer too small");
        close(server_fifo_fd);
        return -1;
    }

    ILOG("Send command for register on server: %s", cmd_register);
    ssize_t written = write(server_fifo_fd, cmd_register, strlen(cmd_register));
    if (written <= 0) {
        ELOG("failed to send command");
        close(server_fifo_fd);
        return -1;
    }

    close(server_fifo_fd);

    // ----------------------------------------------------------------------------------------------


    int temp_tx_fd = open(client->tx_fifo, O_RDONLY | O_NONBLOCK);
    if (temp_tx_fd == -1) {
        ELOG("failed to open tx fifo fd: %s", strerror(errno));
        return -1;
    }

    struct epoll_event ev, events[1] = {0};

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        ELOG("failed epol create");
        close(temp_tx_fd);
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = temp_tx_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, temp_tx_fd, &ev) == -1) {
        ELOG("failed to epoll_ctl");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    int ready = epoll_wait(epoll_fd, events, 1, 5000);
    if (ready == -1) {
        ELOG("epoll_wait failed: %s", strerror(errno));
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    } else if (ready == 0) {
        ELOG("Registration timeout: server did not respond");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    char ack_buffer[32] = "";
    ssize_t n = read(temp_tx_fd, ack_buffer, sizeof(ack_buffer) - 1);
    if (n <= 0) {
        ELOG("failed to read ack");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    ack_buffer[n] = '\0';

    if (strncmp(ack_buffer, RESP_ACK, strlen(RESP_ACK)) != 0) {
        ELOG("incorrectly ack: %s\n", ack_buffer);
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    close(temp_tx_fd);
    close(epoll_fd);
    return 0;
}

int send_get_command(Client *client, const char *filename) {
    if (!client || !filename) {
        ELOG("Invalid arguments for send_get_command");
        return -1;
    }

    char command[512] = "";
    int ret = snprintf(command, sizeof(command), "%s %s\n", CMD_GET, filename);
    if (ret < 0 || ret >= (int)sizeof(command)) {
        ELOG("Failed to format GET command: buffer too small");
        return -1;
    }

    ssize_t written = write(client->rx_fd, command, strlen(command));
    if (written <= 0) {
        ELOG("Failed to send GET command: %s", strerror(errno));
        return -1;
    }

    ILOG("Sent GET command for file: %s", filename);
    return 0;
}

int receive_file(Client *client) {
    if (!client) {
        ELOG("Invalid client argument");
        return -1;
    }

    char buffer[BUFFER_SIZE] = "";
    char size_header[64] = "";
    size_t cur_pos = 0;

    ILOG("Reading file size header");

    while (cur_pos < sizeof(size_header) - 1) {
        ssize_t n = read(client->tx_fd, size_header + cur_pos, 1);
        if (n <= 0) {
            if (n == 0) {
                ELOG("Unexpected EOF while reading size header");
            } else if (errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            } else {
                ELOG("Failed to read size header: %s", strerror(errno));
            }

            if (n == 0 || (n < 0 && errno != EINTR)) {
                return -1;
            }
        }

        if (size_header[cur_pos] == '\n') {
            size_header[cur_pos] = '\0';
            break;
        }

        cur_pos++;
    }

    if (cur_pos >= sizeof(size_header) - 1) {
        ELOG("Size header too long or missing newline");
        return -1;
    }

    size_t file_size = 0;
    if (sscanf(size_header, "SIZE:%zu", &file_size) != 1) {
        ELOG("Invalid size header format: '%s', expected: SIZE:<size>", size_header);
        return -1;
    }

    ILOG("File size: %zu bytes", file_size);
    printf("Size of file: %zu\n", file_size);

    size_t total_received = 0;

    while (total_received < (size_t)file_size) {
        size_t to_read = sizeof(buffer);
        size_t remaining = file_size - total_received;
        if (to_read > remaining) {
            to_read = remaining;
        }

        ssize_t n = read(client->tx_fd, buffer, to_read);
        if (n <= 0) {
            if (n == 0) {
                ELOG("Unexpected EOF while reading file");
            } else if (errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            } else {
                ELOG("Failed to read from server: %s", strerror(errno));
            }

            if (n == 0 || (n < 0 && errno != EINTR)) {
                return -1;
            }
        }

        total_received += (size_t)n;

        if (file_size > 0) {
            double percent = ((double)total_received / (double)file_size) * 100.0;
            printf("GET: %zu/%zu байт (%.1f%%) ", total_received, file_size, percent);
        } else {
            printf("GET: %zu байт ", total_received);
        }

        fflush(stdout);
    }

    return 0;
}
