#ifndef UDP_DISCOVERY_H
#define UDP_DISCOVERY_H

#include <netinet/in.h>
#include "common.h"

#define MAX_SERVERS 64

struct ServerList {
    struct ServerInfo servers[MAX_SERVERS];
    size_t count;
};

int discover_servers_udp(struct ServerList *server_list, int timeout_sec);
int start_udp_discovery_server(void);

#endif // UDP_DISCOVERY_H
