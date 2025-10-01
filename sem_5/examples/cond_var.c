#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_OP 5

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  producer_cond, consumer_cond;
    int             data_ready;
    int             is_busy;
} shared_buffer_t;

typedef struct {
    shared_buffer_t *buffer;
    int             thread_id;
} thread_data_t;

void shared_buf_init(shared_buffer_t *buf) {
    pthread_mutex_init  (&buf->mutex, NULL);
    pthread_cond_init   (&buf->producer_cond, NULL);
    pthread_cond_init   (&buf->consumer_cond, NULL);

    buf->data_ready = 0;
    buf->is_busy = 0;
}

void* produce(void *args) {
    thread_data_t * data    = (thread_data_t*)(args);
    shared_buffer_t *buffer = data->buffer;

    for (int i = 0; i < NUM_OP; i++) {
        pthread_mutex_lock(&buffer->mutex);

        while (buffer->is_busy) {
            printf("Need to wait ... %d\n", data->thread_id);
            pthread_cond_wait(&buffer->producer_cond, &buffer->mutex);
        }

        buffer->is_busy = 1;
        printf("%d: Producing data\n", data->thread_id);
        usleep(500000);
        buffer->data_ready = 1;
        printf("%d: data generated & ready\n", data->thread_id);
        
        buffer->is_busy = 0;
        pthread_mutex_unlock(&buffer->mutex);
        pthread_cond_broadcast(&buffer->consumer_cond);
    }
    
    return NULL;
}

void* consume(void *args) {
    thread_data_t * data    = (thread_data_t*)(args);
    shared_buffer_t *buffer = data->buffer;

    while (1) {
        pthread_mutex_lock(&buffer->mutex);
        while (!buffer->data_ready) {
            printf("Consemer: need to wait ... %d\n", data->thread_id);
            pthread_cond_wait(&buffer->consumer_cond, &buffer->mutex);
        }
        
        buffer->is_busy = 1;
        buffer->data_ready = 0;
        printf("%d: data recieved\n", data->thread_id);
        usleep(500000);
        printf("%d: data processed & ready\n", data->thread_id);
        buffer->is_busy = 0;

        pthread_mutex_unlock(&buffer->mutex);
        pthread_cond_broadcast(&buffer->producer_cond);
    }
    
    return NULL;
}

int main() {
    pthread_t producer, producer2, consumer;
    shared_buffer_t buffer;
    thread_data_t producer_data, consumer_data, producer2_data;

    shared_buf_init(&buffer);

    producer_data.buffer    = &buffer;
    consumer_data.buffer    = &buffer;
    producer2_data.buffer   = &buffer;

    producer_data.thread_id     = 0;
    consumer_data.thread_id     = 1;
    producer2_data.thread_id    = 2;

    pthread_create(&producer, NULL, produce, &producer_data);
    pthread_create(&consumer, NULL, consume, &consumer_data);
    pthread_create(&producer2, NULL, produce, &producer2_data);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    pthread_join(producer2, NULL);

    return 0;
}

