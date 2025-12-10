#ifndef MONTE_CARLO_H
#define MONTE_CARLO_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct SharedData {
    size_t          counter;
    pthread_mutex_t mutex;
};

struct PthreadData {
    double              (*func)(double);
    double              x_min, x_max, y_min, y_max;
    unsigned int        seed;
    size_t              points_per_thread;
    struct SharedData   *shared_data;
    int                 assigned_core;
};

double ExponentialFunc(double x);
double CalculateIntegral(double (*func)(double), int num_threads_sqrt,
                         double x_min, double x_max, double y_min, double y_max);
double CalculateIntegralSimple(double (*func)(double), size_t num_points,
                               double x_min, double x_max, double y_min, double y_max);

#endif // MONTE_CARLO_H
