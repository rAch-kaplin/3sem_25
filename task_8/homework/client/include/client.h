#ifndef CLIENT_H
#define CLIENT_H

#include "config.h"

#define DEFAULT_POINTS_PER_RECTANGLE    10000000
#define DEFAULT_TIMEOUT                 2

typedef struct {
    char    **ip_addresses;
    int     count;
    int     capacity;
} IPList;

IPList* create_iplist();
void add_ip     (IPList *list, const char *ip);
void free_iplist(IPList *list);

int discover_servers(ClientConfig *cfg);
void log_computation_parameters(const ClientConfig *cfg);
int validate_server_count(const ClientConfig *cfg);
double compute_integral_distributed(const ClientConfig *cfg);

#endif // CLIENT_H
