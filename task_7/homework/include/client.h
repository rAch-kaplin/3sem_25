#ifndef CLIENT_H
#define CLIENT_H

#include <signal.h>

typedef struct {
    int     pid;
    int     tx_fd;          // for read from server
    int     rx_fd;          // for write to server
    char    tx_fifo[256];
    char    rx_fifo[256];
} Client;

extern volatile sig_atomic_t should_exit;

void setup_signals(void);
int register_client (Client *client);
int send_get_command(Client *client, const char *filename);
int receive_file    (Client *client);

#endif // CLIENT_H
