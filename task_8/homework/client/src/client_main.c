#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "common.h"
#include "log.h"
#include "client.h"
#include "config.h"

int main(int argc, char *argv[]) {
    if (LoggerInit(LOGL_DEBUG, "client.log", DEFAULT_MODE) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    DLOG_("Client starting...");

    ClientConfig cfg = {0};
    int parse_result = parse_arguments(argc, argv, &cfg);

    if (parse_result > 0) {
        LoggerDeinit();
        return EXIT_SUCCESS;
    } else if (parse_result < 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }

    if (discover_servers(&cfg) <= 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }

    log_computation_parameters(&cfg);

    if (validate_server_count(&cfg) < 0) {
        LoggerDeinit();
        return EXIT_FAILURE;
    }
    struct timespec start = {}, end = {};
    clock_gettime(CLOCK_MONOTONIC, &start);

    double final_result = compute_integral_distributed(&cfg);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (double)(end.tv_sec  - start.tv_sec) +
                      (double)(end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n=== Final Result ===\n");
    printf("Time duration: %lg\n", duration);
    printf("Integral of e^x over [0,1] x [0,e]: %.15lf\n", final_result);
    printf("Analytical value (for comparison): %.15lf\n", (double)exp(1) - 1);
    printf("Absolute error: %.15lf\n", fabs(final_result - ((double)exp(1) - 1)));
    printf("Computed using %d rectangles\n", cfg.num_rectangles_sqrt * cfg.num_rectangles_sqrt);

    DLOG_("Client finished successfully");
    LoggerDeinit();

    return EXIT_SUCCESS;
}
