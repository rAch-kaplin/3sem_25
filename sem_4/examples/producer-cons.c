#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define K 2  // Number of producers
#define N 4  // Number of consumers
#define BUFFER_SIZE 5

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consume = PTHREAD_COND_INITIALIZER;

int buffer = 0;
int running = 1;
int items_produced = 0;
int items_consumed = 0;
const int MAX_ITEMS_PER_PRODUCER = 100000;

// Signal handler for interactive shutdown

void handle_signal(int sig) {
    printf("\nShutting down...\n");
    running = 0;
    // Wake up all waiting threads (so they can check the exit condition)
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond_produce);
    pthread_cond_broadcast(&cond_consume);
    pthread_mutex_unlock(&mutex);
    // Print whole stats on items
    printf("%d, %d\n", items_produced, items_consumed);
}


void* producer(void* arg) {
    int id = *(int*)arg;
    int count = 0;
    
    // Produce while running AND haven't reached individual limit
    while (running && count < MAX_ITEMS_PER_PRODUCER) {
        pthread_mutex_lock(&mutex);
        
        // Wait if buffer is full AND still running
        while (buffer >= BUFFER_SIZE && running) {
            printf("Producer %d: buffer is full. Waiting\n", id);
            pthread_cond_wait(&cond_produce, &mutex);
        }
        
        // Check if we should stop after waking up
        if (!running) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        // Produce item
        buffer++;
        count++;
        items_produced++;
        printf("Producer %d: produced item %d, buffer = %d\n", id, count, buffer);
        
        // Signal consumers that data is available
        pthread_cond_signal(&cond_consume);
        pthread_mutex_unlock(&mutex);
        
        // Simulate variable work time
        usleep((rand() % 100));
    }
    
    printf("Producer %d: finished after %d items\n", id, count);
    free(arg);
    return NULL;
}

void* consumer(void* arg) {
    int id = *(int*)arg;
    int count = 0;
    
    // Continue while running OR there's still work with buffer
    while (running || buffer > 0) {
        pthread_mutex_lock(&mutex);
        
        // Wait only if no work AND might get more (still running)
        while (buffer == 0 && running) {
            printf("Consumer %d: buffer is empty. Waiting\n", id);
            pthread_cond_wait(&cond_consume, &mutex);
        }
        
        // Check if we woke up and there's work to do
        if (buffer > 0) {
            buffer--;
            count++;
            items_consumed++;
            printf("Consumer %d: consumed item %d, buffer = %d\n", id, count, buffer);
            
            // Signal producers that space is available
            pthread_cond_signal(&cond_produce);
        }
        
        pthread_mutex_unlock(&mutex);
        
        // Simulate variable work time (consumers are slower)
        usleep((rand() % 1500));
    }
    
    printf("Consumer %d: finished after %d items\n", id, count);
    free(arg);
    return NULL;
}

int main() {
    pthread_t producers[K];
    pthread_t consumers[N];
    
    printf("Starting %d producers and %d consumers\n", K, N);
    printf("Each producer will create %d items\n", MAX_ITEMS_PER_PRODUCER);
    printf("Buffer size: %d\n\n", BUFFER_SIZE);
    
    // Create producers
    for (int i = 0; i < K; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&producers[i], NULL, producer, id) != 0) {
            perror("Failed to create producer thread");
            exit(1);
        }
    }
    signal(SIGINT, handle_signal); // new for us -- in the next seminar
    // Create consumers
    for (int i = 0; i < N; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&consumers[i], NULL, consumer, id) != 0) {
            perror("Main thread: Failed to create consumer thread");
            exit(1);
        }
    }
   
    for (int i = 0; i < K; i++) {
        pthread_join(producers[i], NULL);
    }
    if (buffer != 0)
        printf("Main Thread: All producers finished. Waiting for consumers to process remaining items...\n");
    
    running = 0;
    
    for (int i = 0; i < N; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_produce);
    pthread_cond_destroy(&cond_consume);
    
    printf("\n=== Final Summary ===\n");
    printf("Total items produced: %d\n", items_produced);
    printf("Total items consumed: %d\n", items_consumed);
    printf("Final buffer state: %d items\n", buffer);
    printf("All threads completed successfully\n");
    
    return 0;
}
