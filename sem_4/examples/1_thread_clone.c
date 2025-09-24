#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define STACK_SIZE (1024 * 1024)    // Stack size for the thread

// Thread function
int thread_function(void *arg) {
    char *message = (char *)arg;
    printf("Thread: %s (PID: %d, TID: %d)\n", message, getpid(), gettid());
    printf("Thread: Shared memory address: %p\n", &message);
    sleep(2);
    return 0;
}

int main() {
    printf("Main process PID: %d\n", getpid());
    
    // Allocate stack for the thread
    void *thread_stack = malloc(STACK_SIZE);
    if (!thread_stack) {
        perror("malloc");
        exit(1);
    }
    // CLONE_VM: Share memory space
    // CLONE_FS: Share filesystem information
    // CLONE_FILES: Share file descriptors
    // CLONE_SIGHAND: Share signal handlers
    // CLONE_THREAD: Create in same thread group
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
    
    pid_t tid = clone(thread_function, 
                     (char *)thread_stack + STACK_SIZE,  // Stack grows downward
                     flags, 
                     (void *)"Hello from thread!");
    
    if (tid == -1) {
        perror("clone");
        free(thread_stack);
        exit(1);
    }
    
    printf("Main: Created thread with TID: %d\n", tid);
    printf("Main: Shared memory address: %p\n", &thread_stack);
    
    // Wait a bit for the thread to complete
    sleep(3);
    
    free(thread_stack);
    printf("Main: Thread completed\n");
    return 0;
}
