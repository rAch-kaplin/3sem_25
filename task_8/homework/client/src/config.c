#include "config.h"
#include "client.h"
#include "log.h"

#include <unistd.h>
#include <getopt.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -b              Use UDP broadcast discovery (default)\n");
    printf("  -i <ip1> [ip2] ...  Use specified IP addresses\n");
    printf("  -n <sqrt>       Square root of number of rectangles (default: 2, creates 4 rectangles)\n");
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

    cfg->server_list.count = 0;
    for (int i = 0; i < iplist->count && cfg->server_list.count < MAX_SERVERS; i++) {
        struct in_addr addr = {0};
        if (inet_aton(iplist->ip_addresses[i], &addr) == 0) {
            ELOG_("Invalid IP address: %s", iplist->ip_addresses[i]);
            free_iplist(iplist);
            return -1;
        }
        cfg->server_list.servers[cfg->server_list.count].addr = addr;
        cfg->server_list.count++;
    }

    free_iplist(iplist);
    return cfg->server_list.count;
}

int parse_arguments(int argc, char *argv[], ClientConfig *cfg) {
    cfg->use_broadcast = 1;
    cfg->num_rectangles_sqrt = DEFAULT_NUM_RECTANGLES_SQRT;
    cfg->points_per_rectangle = DEFAULT_POINTS_PER_RECTANGLE;
    cfg->timeout = DEFAULT_TIMEOUT;
    cfg->x_min = 0.0;
    cfg->x_max = 1.0;
    cfg->y_min = 0.0;
    cfg->y_max = ExponentialFunc(1);

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
            case 'n':
                cfg->num_rectangles_sqrt = atoi(optarg);
                if (cfg->num_rectangles_sqrt < 1) {
                    ELOG_("Number of rectangles sqrt must be >= 1");
                    return -1;
                }
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

            if (inet_aton(ip_arg, &addr) == 0) {
                ELOG_("Invalid IP address: %s", ip_arg);
                return -1;
            }

            cfg->server_list.servers[cfg->server_list.count].addr = addr;
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
