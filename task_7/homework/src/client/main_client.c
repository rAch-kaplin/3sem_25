#include "common.h"
#include "client.h"
#include "log.h"
#include "fifo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (LoggerInit(LOGL_INFO, "client.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    if (argc < 2) {
        ELOG("Usage: %s <file1> [file2] ...", argv[0]);
        LoggerDeinit();
        return 1;
    }

    ILOG("Starting client, PID: %d", getpid());

    setup_signals();

    Client client = {0};
    client.pid = getpid();

    int n = snprintf(client.tx_fifo, sizeof(client.tx_fifo),
                        "%s%d/tx", CLIENT_PREFIX, client.pid);
    if (n < 0 || n >= (int)sizeof(client.tx_fifo)) {
        ELOG("Failed to format TX FIFO path");
        LoggerDeinit();
        return 1;
    }

    n = snprintf(client.rx_fifo, sizeof(client.rx_fifo),
                        "%s%d/rx", CLIENT_PREFIX, client.pid);
    if (n < 0 || n >= (int)sizeof(client.rx_fifo)) {
        ELOG("Failed to format RX FIFO path");
        LoggerDeinit();
        return 1;
    }

    ILOG("Creating FIFOs: tx=%s, rx=%s", client.tx_fifo, client.rx_fifo);

    if (create_fifo_with_path(client.tx_fifo) != 0) {
        ELOG("Failed to create TX FIFO: %s", strerror(errno));
        LoggerDeinit();
        return 1;
    }

    if (create_fifo_with_path(client.rx_fifo) != 0) {
        ELOG("Failed to create RX FIFO: %s", strerror(errno));
        cleanup_fifo(client.tx_fifo);
        LoggerDeinit();
        return 1;
    }

    if (!file_exists(SERVER_FIFO_PATH)) {
        ELOG("Server FIFO does not exist: %s", SERVER_FIFO_PATH);
        cleanup_fifo(client.tx_fifo);
        cleanup_fifo(client.rx_fifo);
        LoggerDeinit();
        return 1;
    }

    if (register_client(&client) != 0) {
        ELOG("Failed to register client");
        cleanup_fifo(client.tx_fifo);
        cleanup_fifo(client.rx_fifo);
        LoggerDeinit();
        return 1;
    }

    ILOG("Client registered successfully");

    client.tx_fd = open(client.tx_fifo, O_RDONLY);
    if (client.tx_fd == -1) {
        ELOG("Failed to open TX FIFO: %s", strerror(errno));
        cleanup_fifo(client.tx_fifo);
        cleanup_fifo(client.rx_fifo);
        LoggerDeinit();
        return 1;
    }

    client.rx_fd = open(client.rx_fifo, O_WRONLY);
    if (client.rx_fd == -1) {
        ELOG("Failed to open RX FIFO: %s", strerror(errno));
        close(client.tx_fd);
        cleanup_fifo(client.tx_fifo);
        cleanup_fifo(client.rx_fifo);
        LoggerDeinit();
        return 1;
    }

    ILOG("Processing %d file(s)", argc - 1);

    for (int i = 1; i < argc; i++) {
        if (should_exit) {
            ILOG("Received exit signal, stopping");
            break;
        }

        ILOG("Requesting file: %s", argv[i]);

        if (send_get_command(&client, argv[i]) != 0) {
            WLOG("Failed to send GET command for file: %s", argv[i]);
            continue;
        }

        if (receive_file(&client) != 0) {
            WLOG("Failed to receive file: %s", argv[i]);
            continue;
        }

        ILOG("Successfully received file: %s", argv[i]);
    }

    ILOG("Sending DISCONNECT command");
    ssize_t written = write(client.rx_fd, CMD_DISCONNECT "\n", strlen(CMD_DISCONNECT) + 1);
    if (written <= 0) {
        ELOG("Failed to send DISCONNECT: %s", strerror(errno));
    }

    close(client.tx_fd);
    close(client.rx_fd);

    cleanup_fifo(client.tx_fifo);
    cleanup_fifo(client.rx_fifo);

    ILOG("Client shutting down");
    LoggerDeinit();

    return 0;
}
