#define _GNU_SOURCE 
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
sem_t sem;

int set_this_thread_to_core(int core_num) {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Total number of cores: %d\n", num_cores);
    if (core_num < 0 || core_num >= num_cores) {
        return EINVAL;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_num, &cpuset);

    pthread_t current = pthread_self();
    return pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
}

void* f(void* args) {
    int core_num = *((int*)(args));
    set_this_thread_to_core(core_num);
    while (1) {
        //sem_wait(&sem);
        ;//printf("Critical section\n");
        //sem_post(&sem); 
    }
    
    return NULL;
}

int main() { 

    sem_init(&sem, 0, 1);
    pthread_t p1, p2;
    int nc[2] = {0};
    nc[0] = 1;
    nc[1] = 8;
    pthread_create(&p1, NULL, f, &nc[0]);
    pthread_create(&p2, NULL, f, &nc[1]);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    sem_destroy(&sem);

    return 0;
}   

