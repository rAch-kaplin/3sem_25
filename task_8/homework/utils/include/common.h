#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <netinet/in.h>

#define UDP_DISCOVERY_PORT 8123
#define TCP_TASK_PORT      8125

#define DISCOVERY_REQUEST  "MONTE_CARLO_DISCOVERY"
#define DISCOVERY_RESPONSE "MONTE_CARLO_READY"

struct Task {
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    size_t num_points;
};

struct Result {
    double integral_value;
    size_t points_inside;
    size_t total_points;
};

struct ServerInfo {
    struct in_addr addr;
};

int serialize_task(const struct Task *task, char *buffer, size_t buffer_size);
int deserialize_task(const char *buffer, struct Task *task);
int serialize_result(const struct Result *result, char *buffer, size_t buffer_size);
int deserialize_result(const char *buffer, struct Result *result);

#endif // COMMON_H
