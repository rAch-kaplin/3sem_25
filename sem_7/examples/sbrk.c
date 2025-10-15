#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    void *current = sbrk(0);
    printf("Start: %p\n", current);

    void *small_array = malloc(4096);
    free(small_array);
    current = sbrk(0);
    printf("Current: %p\n", current);
    
    return 0;
}
