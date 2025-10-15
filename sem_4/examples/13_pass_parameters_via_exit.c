#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct result {
    int value;
    char message[50];
};

void* worker(void* arg) {
    struct result* res = malloc(sizeof(struct result));
    res->value = 100;
    strcpy(res->message, "Operation successful");
    pthread_exit((void*)res);
}

int main() {
    struct result *result;
    void *p;
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
  
    if (pthread_join(t, &p) != 0) {
        perror("Failed to join thread1");
        return 1;
    }

    result = (struct result*)p;
    printf("%d %s\n", result->value, result->message);

    free(result);

    return 0;
}
