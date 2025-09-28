#include <pthread.h>
#include <stdio.h>
#include "stdlib.h"

struct PthreadData {
    double          (*func)(double);
    double          x_min, x_max, y_min, y_max;
    unsigned int    seed;
    size_t          points_per_thread;
};

double ExponentialFunc      (double x);
double CalculateIntegral    (double (*func)(double),int num_threads_sqrt,
                             double x_min, double x_max, double y_min, double y_max);
