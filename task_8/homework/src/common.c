#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

int serialize_task(const struct Task *task, char *buffer, size_t buffer_size) {
    if (!task || !buffer) {
        return -1;
    }

    int written = snprintf(buffer, buffer_size,
                          "TASK:%.15lf:%.15lf:%.15lf:%.15lf:%zu",
                          task->x_min, task->x_max,
                          task->y_min, task->y_max,
                          task->num_points);

    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }

    return written;
}

int deserialize_task(const char *buffer, struct Task *task) {
    if (!buffer || !task) {
        return -1;
    }

    if (strncmp(buffer, "TASK:", 5) != 0) {
        return -1;
    }

    const char *data = buffer + 5;
    int parsed = sscanf(data, "%lf:%lf:%lf:%lf:%zu",
                       &task->x_min, &task->x_max,
                       &task->y_min, &task->y_max,
                       &task->num_points);

    if (parsed != 5) {
        return -1;
    }

    return 0;
}

int serialize_result(const struct Result *result, char *buffer, size_t buffer_size) {
    if (!result || !buffer) {
        return -1;
    }

    int written = snprintf(buffer, buffer_size,
                          "RESULT:%.15lf:%zu:%zu",
                          result->integral_value,
                          result->points_inside,
                          result->total_points);

    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }

    return written;
}

int deserialize_result(const char *buffer, struct Result *result) {
    if (!buffer || !result) {
        return -1;
    }

    if (strncmp(buffer, "RESULT:", 7) != 0) {
        return -1;
    }

    const char *data = buffer + 7;
    int parsed = sscanf(data, "%lf:%zu:%zu",
                       &result->integral_value,
                       &result->points_inside,
                       &result->total_points);

    if (parsed != 3) {
        return -1;
    }

    return 0;
}
