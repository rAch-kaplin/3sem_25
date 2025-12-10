#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "common.h"

int send_task_to_server(const struct ServerInfo *server, const struct Task *task, struct Result *result);

#endif // TCP_CLIENT_H
