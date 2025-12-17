#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdint.h>

/* Запускает TCP сервер задач на указанном порту. */
int start_tcp_task_server(uint16_t port);

#endif // TCP_SERVER_H
