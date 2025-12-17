#include "config.h"
#include "client.h"
#include "log.h"

#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>

static int parse_ip_port(const char *input, struct in_addr *addr, uint16_t *port) {
    char buf[128] = {0};
    char *colon = NULL;

    if (!input || !addr || !port) {
        return -1;
    }

    strncpy(buf, input, sizeof(buf) - 1);
    colon = strchr(buf, ':');

    if (colon) {
        *colon = '\0';
        const char *port_str = colon + 1;
        long p = strtol(port_str, NULL, 10);
        if (p <= 0 || p > 65535) {
            ELOG_("Invalid port in '%s'", input);
            return -1;
        }
        *port = (uint16_t)p;
    } else {
        *port = TCP_TASK_PORT;
    }

    if (inet_pton(AF_INET, buf, addr) == 0) {
        ELOG_("Invalid IP address: %s", buf);
        return -1;
    }

    return 0;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -b                  Use UDP broadcast discovery (default)\n");
    printf("  -i <ip[:port]> [...]  Use specified IP addresses (default port %d)\n", TCP_TASK_PORT);
    printf("  -p <points>     Total points per rectangle (default: 1000000)\n");
    printf("  -t <timeout>     UDP discovery timeout in seconds (default: 2)\n");
    printf("  -h              Show this help\n");
    printf("\n");
    printf("The integral is computed over [0.0, 1.0] x [0.0, e] for function e^x\n");
    printf("The area is divided into n*n rectangles, where n is specified by -n option\n");
}

int parse_ip_addresses_from_args(int argc, char *argv[], int start_idx, ClientConfig *cfg) {
    IPList *iplist = create_iplist();
    if (!iplist) return -1;

    for (int i = start_idx; i < argc; i++) {
        if (argv[i][0] == '-') break;
        add_ip(iplist, argv[i]);
    }

<<<<<<< HEAD
    cfg->server_list.count = 0;
    for (int i = 0; i < iplist->count && cfg->server_list.count < MAX_SERVERS; i++) {
        struct in_addr addr = {0};
        if (inet_pton(AF_INET, iplist->ip_addresses[i], &addr) == 0) {
            ELOG_("Invalid IP address: %s", iplist->ip_addresses[i]);
            free_iplist(iplist);
            return -1;
        }
        cfg->server_list.servers[cfg->server_list.count].addr = addr;
=======
    for (int i = 0; i < iplist->count && cfg->server_list.count < MAX_SERVERS; i++) {
        struct in_addr addr = {0};
        uint16_t port = 0;

        if (parse_ip_port(iplist->ip_addresses[i], &addr, &port) < 0) {
            free_iplist(iplist);
            return -1;
        }

        cfg->server_list.servers[cfg->server_list.count].addr = addr;
        cfg->server_list.servers[cfg->server_list.count].port = port;
>>>>>>> task-8
        cfg->server_list.count++;
    }

    free_iplist(iplist);
    return cfg->server_list.count;
}

int parse_arguments(int argc, char *argv[], ClientConfig *cfg) {
    cfg->use_broadcast = 1;
    cfg->points_per_rectangle = DEFAULT_POINTS_PER_RECTANGLE;
    cfg->timeout = DEFAULT_TIMEOUT;
    cfg->x_min = 0.0;
    cfg->x_max = 1.0;
    cfg->y_min = 0.0;
    cfg->y_max = exp(1);

    cfg->server_list.count = 0;

    int opt = -1;
    int ip_option_seen = 0;
    char *ip_arg = NULL;

    while ((opt = getopt(argc, argv, "bi:n:p:t:h")) != -1) {
        switch (opt) {
            case 'b':
                cfg->use_broadcast = 1;
                break;
            case 'i':
                cfg->use_broadcast = 0;
                ip_option_seen = 1;
                ip_arg = optarg;
                break;
            case 'p':
                cfg->points_per_rectangle = (size_t)atoll(optarg);
                if (cfg->points_per_rectangle == 0) {
                    ELOG_("Points per rectangle must be > 0");
                    return -1;
                }
                break;
            case 't':
                cfg->timeout = atoi(optarg);
                if (cfg->timeout <= 0) {
                    ELOG_("Timeout must be > 0");
                    return -1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 1;
            default:
                ELOG_("Invalid option");
                return -1;
        }
    }

    if (ip_option_seen) {
        if (ip_arg) {
            struct in_addr addr = {0};
<<<<<<< HEAD

            if (inet_pton(AF_INET, ip_arg, &addr) == 0) {
                ELOG_("Invalid IP address: %s", ip_arg);
=======
            uint16_t port = 0;

            if (parse_ip_port(ip_arg, &addr, &port) < 0) {
>>>>>>> task-8
                return -1;
            }

            cfg->server_list.servers[cfg->server_list.count].addr = addr;
<<<<<<< HEAD
=======
            cfg->server_list.servers[cfg->server_list.count].port = port;
>>>>>>> task-8
            cfg->server_list.count++;
        }

        if (optind < argc) {
            if (parse_ip_addresses_from_args(argc, argv, optind, cfg) < 0) {
                return -1;
            }
        }

        if (cfg->server_list.count == 0) {
            ELOG_("-i option requires at least one IP address");
            return -1;
        }
    }

    return 0;
}
