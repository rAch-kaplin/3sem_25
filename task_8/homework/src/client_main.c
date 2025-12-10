#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"
#include "udp_discovery.h"
#include "tcp_client.h"
#include "monte_carlo.h"

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

static int parse_ip_addresses(int argc, char *argv[], int start_idx,
                              struct ServerList *server_list) {
    server_list->count = 0;

    for (int i = start_idx; i < argc && server_list->count < MAX_SERVERS; i++) {
        if (argv[i][0] == '-') {
            break;
        }

        struct in_addr addr;
        if (inet_aton(argv[i], &addr) == 0) {
            fprintf(stderr, "Invalid IP address: %s\n", argv[i]);
            return -1;
        }

        server_list->servers[server_list->count].addr = addr;
        server_list->servers[server_list->count].num_cores = 1;
        server_list->count++;
    }

    return (int)server_list->count;
}

int main(int argc, char *argv[]) {
    int use_broadcast = 1;
    struct ServerList server_list = {0};
    double x_min = 0.0, x_max = 1.0;
    double y_min = 0.0, y_max = 2.718281828459045;

    int num_rectangles_sqrt = 2;
    size_t points_per_rectangle = 1000000;
    int timeout = 2;

    int opt = 0;
    int ip_option_seen = 0;
    while ((opt = getopt(argc, argv, "bi:n:p:t:h")) != -1) {
        switch (opt) {
            case 'b':
                use_broadcast = 1;
                break;
            case 'i':
                use_broadcast = 0;
                ip_option_seen = 1;
                break;
            case 'n':
                num_rectangles_sqrt = atoi(optarg);
                if (num_rectangles_sqrt < 1) {
                    fprintf(stderr, "Error: number of rectangles sqrt must be >= 1\n");
                    return 1;
                }
                break;
            case 'p':
                points_per_rectangle = (size_t)atoll(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (ip_option_seen) {
        if (optind >= argc) {
            fprintf(stderr, "Error: -i option requires at least one IP address\n");
            print_usage(argv[0]);
            return 1;
        }
        if (parse_ip_addresses(argc, argv, optind, &server_list) <= 0) {
            fprintf(stderr, "Error: failed to parse IP addresses\n");
            return 1;
        }
    }

    if (use_broadcast) {
        printf("Discovering servers via UDP broadcast...\n");
        if (discover_servers_udp(&server_list, timeout) <= 0) {
            fprintf(stderr, "No servers found via UDP broadcast\n");
            return 1;
        }
    } else {
        if (server_list.count == 0) {
            fprintf(stderr, "No IP addresses specified\n");
            print_usage(argv[0]);
            return 1;
        }
        printf("Using %zu specified server(s)\n", server_list.count);
    }

    if (server_list.count == 0) {
        fprintf(stderr, "No servers available\n");
        return 1;
    }

    int total_rectangles = num_rectangles_sqrt * num_rectangles_sqrt;

    printf("\nComputing integral from [%.3f, %.3f] x [%.3f, %.3f]\n",
           x_min, x_max, y_min, y_max);
    printf("Requested grid: %dx%d = %d rectangles\n",
           num_rectangles_sqrt, num_rectangles_sqrt, total_rectangles);
    printf("Available servers: %zu\n", server_list.count);
    printf("Points per rectangle: %zu\n", points_per_rectangle);

    size_t rectangles_to_use = (server_list.count < (size_t)total_rectangles)
                                ? server_list.count
                                : (size_t)total_rectangles;

    if (server_list.count < (size_t)total_rectangles) {
        printf("Warning: Not enough servers! Requested %d rectangles, but only %zu servers available.\n",
               total_rectangles, server_list.count);
        printf("Using only %zu rectangles (first %zu servers).\n",
               rectangles_to_use, rectangles_to_use);
    } else {
        printf("Using all %d rectangles.\n", total_rectangles);
    }
    printf("\n");

    double x_range = x_max - x_min;
    double y_range = y_max - y_min;
    double x_step = x_range / (double)num_rectangles_sqrt;
    double y_step = y_range / (double)num_rectangles_sqrt;

    double total_result = 0.0;
    size_t server_idx = 0;

    for (int i = 0; i < num_rectangles_sqrt && server_idx < rectangles_to_use; i++) {
        for (int j = 0; j < num_rectangles_sqrt && server_idx < rectangles_to_use; j++) {
            double rect_x_min = x_min + i * x_step;
            double rect_x_max = (i == num_rectangles_sqrt - 1) ? x_max : x_min + (i + 1) * x_step;
            double rect_y_min = y_min + j * y_step;
            double rect_y_max = (j == num_rectangles_sqrt - 1) ? y_max : y_min + (j + 1) * y_step;

            struct Task task = {0};
            task.x_min = rect_x_min;
            task.x_max = rect_x_max;
            task.y_min = rect_y_min;
            task.y_max = rect_y_max;
            task.num_points = points_per_rectangle;

            printf("Sending rectangle [%d,%d] to %s (interval: [%.3f, %.3f] x [%.3f, %.3f], points: %zu)...\n",
                   i, j, inet_ntoa(server_list.servers[server_idx].addr),
                   task.x_min, task.x_max, task.y_min, task.y_max,
                   task.num_points);

            struct Result result = {0};
            if (send_task_to_server(&server_list.servers[server_idx], &task, &result) < 0) {
                fprintf(stderr, "Failed to get result from %s\n",
                        inet_ntoa(server_list.servers[server_idx].addr));
                server_idx++;
                continue;
            }

            printf("  Partial result: %.15lf (points inside: %zu/%zu)\n",
                   result.integral_value, result.points_inside, result.total_points);

            total_result += result.integral_value;
            server_idx++;
        }
    }

    printf("\n=== Final Result ===\n");
    printf("Integral value: %.15lf\n", total_result);
    printf("Computed using %zu rectangles out of %d requested\n",
           rectangles_to_use, total_rectangles);

    return 0;
}
