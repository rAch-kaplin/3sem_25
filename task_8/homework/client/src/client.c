#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "client.h"
#include "config.h"
#include "log.h"
#include "udp_discovery.h"
#include "tcp_client.h"

typedef struct {
    const struct ServerInfo *server;
    struct Task task;
    struct Result result;
    int status;
} TaskJob;

static void* task_worker(void *arg) {
    TaskJob *job = (TaskJob*)arg;
    struct Result res = {0};

    if (send_task_to_server(job->server, &job->task, &res) < 0) {
        job->status = -1;
        return NULL;
    }

    job->result = res;
    job->status = 0;
    return NULL;
}

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
        list->ip_addresses = (char**)realloc(list->ip_addresses,
                                             list->capacity * sizeof(char*));
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

    size_t num_strips = cfg->server_list.count;
    if (num_strips == 0) {
        ELOG_("No servers available");
        return 0.0;
    }

    double x_range = cfg->x_max - cfg->x_min;
    double y_range = cfg->y_max - cfg->y_min;

    double strip_width = x_range / num_strips;

    /* Общее число точек, указанное пользователем, делим между серверами,
     * чтобы при увеличении числа серверов не рос суммарный объём работы. */
    size_t total_points = cfg->points_per_rectangle;
    size_t base_points_per_strip = total_points / num_strips;
    size_t remainder = total_points % num_strips;

    TaskJob *jobs = (TaskJob*)calloc(num_strips, sizeof(*jobs));
    if (!jobs) {
        ELOG_("Failed to allocate jobs array");
        return 0.0;
    }

    pthread_t *threads = (pthread_t*)calloc(num_strips, sizeof(*threads));
    if (!threads) {
        ELOG_("Failed to allocate threads array");
        free(jobs);
        return 0.0;
    }

    for (size_t strip_idx = 0; strip_idx < num_strips; strip_idx++) {
        double strip_x_min = cfg->x_min + strip_idx * strip_width;
        double strip_x_max = (strip_idx == num_strips - 1)
                            ? cfg->x_max
                            : strip_x_min + strip_width;

        size_t points_for_strip = base_points_per_strip +
                                  (strip_idx < remainder ? 1 : 0);

        jobs[strip_idx].server = &cfg->server_list.servers[strip_idx];
        jobs[strip_idx].task.x_min = strip_x_min;
        jobs[strip_idx].task.x_max = strip_x_max;
        jobs[strip_idx].task.y_min = cfg->y_min;
        jobs[strip_idx].task.y_max = cfg->y_max;
        jobs[strip_idx].task.num_points = points_for_strip;
        jobs[strip_idx].status = -1;

        DLOG_("Sending strip %zu to %s (x: [%.3f, %.3f], y: [%.3f, %.3f], points: %zu)",
               strip_idx, inet_ntoa(jobs[strip_idx].server->addr),
               jobs[strip_idx].task.x_min, jobs[strip_idx].task.x_max,
               jobs[strip_idx].task.y_min, jobs[strip_idx].task.y_max,
               jobs[strip_idx].task.num_points);

        int rc = pthread_create(&threads[strip_idx], NULL, task_worker, &jobs[strip_idx]);
        if (rc != 0) {
            ELOG_("pthread_create failed: %s", strerror(rc));
            jobs[strip_idx].status = -1;
        }
    }

    double total_result = 0.0;

    for (size_t i = 0; i < num_strips; i++) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
        }

        if (jobs[i].status == 0) {
            DLOG_("Partial result from %s: %.15lf (points inside: %zu/%zu)",
                   inet_ntoa(jobs[i].server->addr),
                   jobs[i].result.integral_value,
                   jobs[i].result.points_inside,
                   jobs[i].result.total_points);

            total_result += jobs[i].result.integral_value;
        } else {
            ELOG_("Failed to get result from %s",
                   inet_ntoa(jobs[i].server->addr));
        }
    }

    DLOG_("Processed %zu strips successfully", num_strips);

    free(threads);
    free(jobs);

    return total_result;
}
