#ifndef CONFIG_H
#define CONFIG_H

#include "udp_discovery.h"

typedef struct {
    int     use_broadcast;
    struct  ServerList server_list;
    int     num_rectangles_sqrt;
    size_t  points_per_rectangle;
    int     timeout;
    double  x_min, x_max, y_min, y_max;
} ClientConfig;

int parse_arguments(int argc, char *argv[], ClientConfig *config);
void print_usage(const char *prog_name);

#endif // CONFIG_H
