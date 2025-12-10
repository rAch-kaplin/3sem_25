#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "client.h"
#include "config.h"
#include "log.h"
#include "udp_discovery.h"


IPList* create_iplist() {
    IPList *list = (IPList*)calloc(1, sizeof(IPList));
    if (!list) {
        ELOG_("Failed to allocate memory for IP list");
        return NULL;
    }

    list->capacity = 4;
    list->count = 0;
    list->ip_addresses = (char**)calloc(list->capacity, sizeof(char*));

    if (!list->ip_addresses) {
        ELOG_("Failed to allocate memory for IP addresses");
        free(list);
        return NULL;
    }

    return list;
}

void add_ip(IPList *list, const char *ip) {
    assert(list);

    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->ip_addresses = realloc(list->ip_addresses, list->capacity * sizeof(char*));
        if (!list->ip_addresses) {
            ELOG_("Failed to reallocate memory for IP addresses");
            return;
        }
    }

    list->ip_addresses[list->count] = strdup(ip);
    if (!list->ip_addresses[list->count]) {
        ELOG_("Failed to duplicate IP address");
        return;
    }
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

int discover_servers(ClientConfig *cfg) {
    assert(cfg);

    if (cfg->use_broadcast) {
        DLOG_("Discovering servers via UDP broadcast (timeout: %d seconds)...", cfg->timeout);

        int discovered = discover_servers_udp(&cfg->server_list, cfg->timeout);
        if (discovered <= 0) {
            ELOG_("No servers found via UDP broadcast");
            return -1;
        }

        DLOG_("Discovered %d server(s) via UDP broadcast", discovered);

        return discovered;
    } else {
        DLOG_("Using %zu specified server(s)", cfg->server_list.count);

        return cfg->server_list.count;
    }
}

void log_computation_parameters(const ClientConfig *cfg) {
    assert(cfg);

    int total_rectangles = cfg->num_rectangles_sqrt * cfg->num_rectangles_sqrt;

    DLOG_("Computing integral from [%.3f, %.3f] x [%.3f, %.3f]",
           cfg->x_min, cfg->x_max, cfg->y_min, cfg->y_max);

    DLOG_("Requested grid: %dx%d = %d rectangles",
           cfg->num_rectangles_sqrt, cfg->num_rectangles_sqrt, total_rectangles);

    DLOG_("Available servers: %zu", cfg->server_list.count);
}

int validate_server_count(const ClientConfig *cfg) {
    assert(cfg);

    int total_rectangles = cfg->num_rectangles_sqrt * cfg->num_rectangles_sqrt;

    if (cfg->server_list.count == 0) {
        ELOG_("No servers available");
        return -1;
    }

    if (cfg->server_list.count < (size_t)total_rectangles) {
        WLOG_("Warning: Not enough servers! Requested %d rectangles, but only %zu servers available",
               total_rectangles, cfg->server_list.count);

        DLOG_("Using only %zu rectangles (first %zu servers)",
               cfg->server_list.count, cfg->server_list.count);
    } else {
        DLOG_("Using all %d rectangles", total_rectangles);
    }

    return 0;
}

double compute_integral_distributed(const ClientConfig *cfg) {
    assert(cfg);

    int total_rectangles = cfg->num_rectangles_sqrt * cfg->num_rectangles_sqrt;
    size_t rectangles_to_use = (cfg->server_list.count < (size_t)total_rectangles)
                                ? cfg->server_list.count
                                : (size_t)total_rectangles;

    double x_range = cfg->x_max - cfg->x_min;
    double y_range = cfg->y_max - cfg->y_min;

    double x_step = x_range / cfg->num_rectangles_sqrt;
    double y_step = y_range / cfg->num_rectangles_sqrt;

    double total_result = 0.0;
    size_t server_idx = 0;
    int rectangles_processed = 0;

    for (int i = 0; i < cfg->num_rectangles_sqrt && server_idx < rectangles_to_use; i++) {
        for (int j = 0; j < cfg->num_rectangles_sqrt && server_idx < rectangles_to_use; j++) {

            double rect_x_min = cfg->x_min + i * x_step;
            double rect_x_max = (i == cfg->num_rectangles_sqrt - 1) ? cfg->x_max : cfg->x_min + (i + 1) * x_step;

            double rect_y_min = cfg->y_min + j * y_step;
            double rect_y_max = (j == cfg->num_rectangles_sqrt - 1) ? cfg->y_max : cfg->y_min + (j + 1) * y_step;

            struct Task task = {
                .x_min = rect_x_min,
                .x_max = rect_x_max,
                .y_min = rect_y_min,
                .y_max = rect_y_max,
                .num_points = cfg->points_per_rectangle
            };

            DLOG_("Sending rectangle [%d,%d] to %s (interval: [%.3f, %.3f] x [%.3f, %.3f], points: %zu)",
                   i, j, inet_ntoa(cfg->server_list.servers[server_idx].addr),
                   task.x_min, task.x_max, task.y_min, task.y_max,
                   task.num_points);

            struct Result result = {0};
            if (send_task_to_server(&cfg->server_list.servers[server_idx], &task, &result) < 0) {
                ELOG_("Failed to get result from %s",
                       inet_ntoa(cfg->server_list.servers[server_idx].addr));
                server_idx++;
                continue;
            }

            DLOG_("Partial result from %s: %.15lf (points inside: %zu/%zu)",
                   inet_ntoa(cfg->server_list.servers[server_idx].addr),
                   result.integral_value, result.points_inside, result.total_points);

            total_result += result.integral_value;
            rectangles_processed++;
            server_idx++;
        }
    }

    DLOG_("Processed %d rectangles successfully", rectangles_processed);
    return total_result;
}
