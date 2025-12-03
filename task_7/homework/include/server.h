#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

typedef struct {
    int     client_id;
    int     tx_fd;
    int     rx_fd;
    char    tx_path[256];
    char    rx_path[256];
    int     is_active;
} ClientInfo;

typedef struct {
    int     client_fd;
    char    filename[256];
} FileTask;

extern ClientInfo clients[];
extern volatile sig_atomic_t server_running;
extern int epoll_fd;

void setup_server_signals(void);
void server_signal_handler(int sig);
int find_free_client_slot(void);
void close_client_connection(int client_id);
void* send_file_to_client(void *arg);
void process_client_command(int client_id, const char *command);
int register_new_client(const char *tx_path, const char *rx_path);
void process_registration_commands(int server_fifo_fd);

#endif //SERVER_H
