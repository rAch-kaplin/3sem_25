#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "common.h"
#include "udp_discovery.h"
#include "tcp_client.h"
#include "monte_carlo.h"
#include "log.h"

#define DEFAULT_NUM_RECTANGLES_SQRT     1
#define DEFAULT_POINTS_PER_RECTANGLE    10000000
#define DEFAULT_TIMEOUT                 2

static void print_usage(const char *prog_name) {
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

typedef struct {
    int use_broadcast;
    struct ServerList server_list;
    int num_rectangles_sqrt;
    size_t points_per_rectangle;
    int timeout;
    double x_min, x_max, y_min, y_max;
} ClientConfig;

typedef struct {
    char **ip_addresses;
    int count;
    int capacity;
} IPList;

int parse_arguments(int argc, char *argv[], ClientConfig *config);
void print_usage(const char *prog_name);
IPList* create_iplist();
void add_ip(IPList *list, const char *ip);
void free_iplist(IPList *list);

IPList* create_iplist() {
    IPList *list = (IPList*)calloc(1, sizeof(IPList));
    if (!list) return NULL;

    list->capacity = 4;
    list->count = 0;
    list->ip_addresses = (char**)calloc(list->capacity, sizeof(char*));

    return list;
}

void add_ip(IPList *list, const char *ip) {
    assert(list);

    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->ip_addresses = realloc(list->ip_addresses, list->capacity * sizeof(char*));
    }

    list->ip_addresses[list->count] = strdup(ip);
    list->count++;
}

void free_iplist(IPList *list) {
    assert(list);

    for (int i = 0; i < list->count; i++) {
        free(list->ip_addresses[i]);
    }

    free(list->ip_addresses);
    free(list);
}

static int parse_ip_addresses_from_args(int argc, char *argv[], int start_idx, ClientConfig *config) {
    IPList *iplist = create_iplist();
    if (!iplist) return -1;

    for (int i = start_idx; i < argc; i++) {
        if (argv[i][0] == '-') {
            break;
        }
        add_ip(iplist, argv[i]);
    }

    config->server_list.count = 0;
    for (int i = 0; i < iplist->count && config->server_list.count < MAX_SERVERS; i++) {
        struct in_addr addr;
        if (inet_aton(iplist->ip_addresses[i], &addr) == 0) {
            ELOG_("Invalid IP address: %s", iplist->ip_addresses[i]);
            free_iplist(iplist);
            return -1;
        }
        config->server_list.servers[config->server_list.count].addr = addr;
        config->server_list.count++;
    }

    free_iplist(iplist);
    return config->server_list.count;
}

int parse_arguments(int argc, char *argv[], ClientConfig *config) {
    config->use_broadcast = 1;
    config->num_rectangles_sqrt = DEFAULT_NUM_RECTANGLES_SQRT;
    config->points_per_rectangle = DEFAULT_POINTS_PER_RECTANGLE;
    config->timeout = DEFAULT_TIMEOUT;
    config->x_min = 0.0;
    config->x_max = 1.0;
    config->y_min = 0.0;
    config->y_max = ExponentialFunc(1);

    config->server_list.count = 0;

    int opt = -1;
    int ip_option_seen = 0;
    char *ip_arg = NULL;

    while ((opt = getopt(argc, argv, "bi:n:p:t:h")) != -1) {
        switch (opt) {
            case 'b':
                config->use_broadcast = 1;
                break;
            case 'i':
                config->use_broadcast = 0;
                ip_option_seen = 1;
                ip_arg = optarg;
                break;
            case 'n':
                config->num_rectangles_sqrt = atoi(optarg);
                if (config->num_rectangles_sqrt < 1) {
                    ELOG_("Number of rectangles sqrt must be >= 1");
                    return -1;
                }
                break;
            case 'p':
                config->points_per_rectangle = (size_t)atoll(optarg);
                if (config->points_per_rectangle == 0) {
                    ELOG_("Points per rectangle must be > 0");
                    return -1;
                }
                break;
            case 't':
                config->timeout = atoi(optarg);
                if (config->timeout <= 0) {
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

            config->server_list.servers[config->server_list.count].addr = addr;
            config->server_list.count++;
        }

        if (optind < argc) {
            if (parse_ip_addresses_from_args(argc, argv, optind, config) < 0) {
                return -1;
            }
        }

        if (config->server_list.count == 0) {
            ELOG_("-i option requires at least one IP address");
            return -1;
        }
    }

    return 0;
}

int discover_servers(ClientConfig *config) {
    assert(config);

    if (config->use_broadcast) {
        DLOG_("Discovering servers via UDP broadcast (timeout: %d seconds)...", config->timeout);

        int discovered = discover_servers_udp(&config->server_list, config->timeout);
        if (discovered <= 0) {
            ELOG_("No servers found via UDP broadcast");
            return -1;
        }

        DLOG_("Discovered %d server(s) via UDP broadcast", discovered);

        return discovered;
    } else {
        DLOG_("Using %zu specified server(s)", config->server_list.count);

        return config->server_list.count;
    }
}

void log_computation_parameters(const ClientConfig *config) {
    assert(config);

    int total_rectangles = config->num_rectangles_sqrt * config->num_rectangles_sqrt;

    DLOG_("Computing integral from [%.3f, %.3f] x [%.3f, %.3f]",
           config->x_min, config->x_max, config->y_min, config->y_max);

    DLOG_("Requested grid: %dx%d = %d rectangles",
           config->num_rectangles_sqrt, config->num_rectangles_sqrt, total_rectangles);

    DLOG_("Available servers: %zu", config->server_list.count);
}

int validate_server_count(const ClientConfig *config) {
    assert(config);

    int total_rectangles = config->num_rectangles_sqrt * config->num_rectangles_sqrt;

    if (config->server_list.count == 0) {
        ELOG_("No servers available");
        return -1;
    }

    if (config->server_list.count < (size_t)total_rectangles) {
        WLOG_("Warning: Not enough servers! Requested %d rectangles, but only %zu servers available",
               total_rectangles, config->server_list.count);

        DLOG_("Using only %zu rectangles (first %zu servers)",
               config->server_list.count, config->server_list.count);
    } else {
        DLOG_("Using all %d rectangles", total_rectangles);
    }

    return 0;
}

double compute_integral_distributed(const ClientConfig *config) {
    int total_rectangles = config->num_rectangles_sqrt * config->num_rectangles_sqrt;
    size_t rectangles_to_use = (config->server_list.count < (size_t)total_rectangles)
                                ? config->server_list.count
                                : (size_t)total_rectangles;

    double x_range = config->x_max - config->x_min;
    double y_range = config->y_max - config->y_min;
    double x_step = x_range / config->num_rectangles_sqrt;
    double y_step = y_range / config->num_rectangles_sqrt;

    double total_result = 0.0;
    size_t server_idx = 0;
    int rectangles_processed = 0;

    for (int i = 0; i < config->num_rectangles_sqrt && server_idx < rectangles_to_use; i++) {
        for (int j = 0; j < config->num_rectangles_sqrt && server_idx < rectangles_to_use; j++) {
            double rect_x_min = config->x_min + i * x_step;
            double rect_x_max = (i == config->num_rectangles_sqrt - 1) ? config->x_max : config->x_min + (i + 1) * x_step;
            double rect_y_min = config->y_min + j * y_step;
            double rect_y_max = (j == config->num_rectangles_sqrt - 1) ? config->y_max : config->y_min + (j + 1) * y_step;

            struct Task task = {
                .x_min = rect_x_min,
                .x_max = rect_x_max,
                .y_min = rect_y_min,
                .y_max = rect_y_max,
                .num_points = config->points_per_rectangle
            };

            DLOG_("Sending rectangle [%d,%d] to %s (interval: [%.3f, %.3f] x [%.3f, %.3f], points: %zu)",
                   i, j, inet_ntoa(config->server_list.servers[server_idx].addr),
                   task.x_min, task.x_max, task.y_min, task.y_max,
                   task.num_points);

            struct Result result = {0};
            if (send_task_to_server(&config->server_list.servers[server_idx], &task, &result) < 0) {
                ELOG_("Failed to get result from %s",
                       inet_ntoa(config->server_list.servers[server_idx].addr));
                server_idx++;
                continue;
            }

            DLOG_("Partial result from %s: %.15lf (points inside: %zu/%zu)",
                   inet_ntoa(config->server_list.servers[server_idx].addr),
                   result.integral_value, result.points_inside, result.total_points);

            total_result += result.integral_value;
            rectangles_processed++;
            server_idx++;
        }
    }

    DLOG_("Processed %d rectangles successfully", rectangles_processed);
    return total_result;
}

int main(int argc, char *argv[]) {
    if (LoggerInit(LOGL_DEBUG, "client.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    DLOG_("Client starting...");

    ClientConfig config = {0};
    int parse_result = parse_arguments(argc, argv, &config);

    if (parse_result > 0) {
        LoggerDeinit();
        return EXIT_SUCCESS;
    } else if (parse_result < 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }

    if (discover_servers(&config) <= 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }

    log_computation_parameters(&config);

    if (validate_server_count(&config) < 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }

    double final_result = compute_integral_distributed(&config);

    printf("\n=== Final Result ===\n");
    printf("Integral of e^x over [0,1] x [0,e]: %.15lf\n", final_result);
    printf("Analytical value (for comparison): %.15lf\n", ExponentialFunc(1) - 1);
    printf("Absolute error: %.15lf\n", fabs(final_result - (ExponentialFunc(1) - 1)));
    printf("Computed using %d rectangles\n",
           config.num_rectangles_sqrt * config.num_rectangles_sqrt);

    LoggerDeinit();
    DLOG_("Client finished successfully");

    return EXIT_SUCCESS;
}
