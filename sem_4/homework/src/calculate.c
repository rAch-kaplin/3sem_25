#define _GNU_SOURCE
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "monte_carlo.h"

const size_t TOTAL_POINTS = 10000000;

static void* MonteCarloThread(void *args);

double ExponentialFunc(double x) {
    return exp(x);
}

static int set_this_thread_to_core(int core_num);

static void* MonteCarloThread(void *args) {
    assert(args);

    struct PthreadData *data = (struct PthreadData*)args;
    int err = set_this_thread_to_core(data->assigned_core);
    if (err != 0) {
        printf("Failed to create pthread on core\n");
        return NULL;
    }
    size_t local_count  = 0;

    for (size_t i = 0; i < data->points_per_thread; i++) {
        double x = data->x_min + (double)rand_r(&data->seed) / RAND_MAX * (data->x_max - data->x_min);
        double y = data->y_min + (double)rand_r(&data->seed) / RAND_MAX * (data->y_max - data->y_min);

        if (y <= data->func(x)) {
            local_count++;
        }
    }

    pthread_mutex_lock  (&data->shared_data->mutex);
    data->shared_data->counter += local_count;
    pthread_mutex_unlock(&data->shared_data->mutex);

    return NULL;
}

double CalculateIntegral(double (*func)(double),
                         int num_threads_sqrt,
                         double x_min, double x_max,
                         double y_min, double y_max) {
    struct SharedData shared = {0};

    pthread_mutex_init(&shared.mutex, NULL);

    int num_threads = num_threads_sqrt * num_threads_sqrt;
    double scale_step   = 1.0 / num_threads_sqrt;
    double x_step       = (x_max - x_min) * scale_step;
    double y_step       = (y_max - y_min) * scale_step;

    size_t points_per_thread = TOTAL_POINTS / (size_t)num_threads;

    struct PthreadData *data = calloc((size_t)num_threads, sizeof(*data));
    pthread_t *tid           = calloc((size_t)num_threads, sizeof(*tid));

    struct timespec start = {}, end = {};
    clock_gettime(CLOCK_MONOTONIC, &start);

    int k = 0;
    for (int i = 0; i < num_threads_sqrt; i++) {
        for (int j = 0; j < num_threads_sqrt; j++) {
            int index = i * num_threads_sqrt + j;

            data[index].x_min               = x_min + i * x_step;
            data[index].x_max               = x_min + (i + 1) * x_step;
            data[index].y_min               = y_min + j * y_step;
            data[index].y_max               = y_min + (j + 1) * y_step;
            data[index].func                = func;
            data[index].points_per_thread   = points_per_thread;
            data[index].seed                = (unsigned int)time(NULL) + (unsigned int)index;
            data[index].shared_data         = &shared;
            data[index].assigned_core       = k;

            #if 0
            printf("Thread %d: [%.3f,%.3f]x[%.3f,%.3f]\n",
                   index, data[index].x_min, data[index].x_max,
                   data[index].y_min, data[index].y_max);
            #endif

            pthread_create(&tid[k], NULL, MonteCarloThread, &data[index]);
            k++;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&shared.mutex);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (double)(end.tv_sec  - start.tv_sec) +
                      (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time duration: %lg\n", duration);

    free(data);
    free(tid);

    return (x_max - x_min) * (y_max - y_min) * (double)shared.counter / (double)TOTAL_POINTS;
}

int set_this_thread_to_core(int core_num) {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    //printf("Total number of cores: %d\n", num_cores);
    if (core_num < 0 || core_num >= num_cores) {
        return -1;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_num, &cpuset);

    pthread_t current = pthread_self();
    return pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
}

