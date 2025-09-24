#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_ITERATIONS 1000000
#define NUM_THREADS 4

pthread_mutex_t mutex;
pthread_spinlock_t spinlock;
int counter_mutex = 0;
int counter_spinlock = 0;

void* test_mutex(void* arg) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        counter_mutex++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* test_spinlock(void* arg) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        pthread_spin_lock(&spinlock);
        counter_spinlock++;
        pthread_spin_unlock(&spinlock);
    }
    return NULL;
}

void benchmark(void* (*func)(void*), const char* name) {
    pthread_t threads[NUM_THREADS];
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, func, NULL);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("%s: Time = %.6f seconds\n", name, time_taken);
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
    
    printf("Benchmarking with %d threads, %d iterations each:\n", 
           NUM_THREADS, NUM_ITERATIONS);
    
    benchmark(test_mutex, "Mutex");
    benchmark(test_spinlock, "Spinlock");
    
    printf("Mutex counter: %d\n", counter_mutex);
    printf("Spinlock counter: %d\n", counter_spinlock);
    
    pthread_mutex_destroy(&mutex);
    pthread_spin_destroy(&spinlock);
    
    return 0;
}
