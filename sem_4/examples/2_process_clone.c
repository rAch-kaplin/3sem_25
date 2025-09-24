#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define STACK_SIZE (1024 * 1024)    // Stack size for the child process

// Child process function
int child_function(void *arg) {
    char *message = (char *)arg;
    printf("Child process: %s (PID: %d, PPID: %d)\n", message, getpid(), getppid());
    printf("Child: Memory address: %p\n", &message);
    
    // Modify a variable to show separate memory space
    int local_var = 100;
    printf("Child: Local variable address: %p, value: %d\n", &local_var, local_var);
    
    sleep(2);
    return 42;  // Exit status
}

int main() {
    printf("Parent process PID: %d\n", getpid());
    
    // Allocate stack for the child process
    void *child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("malloc");
        exit(1);
    }
    
    // Create a new process using clone with minimal sharing
    // No CLONE_VM: Separate memory space
    // No CLONE_THREAD: Different thread group
    // SIGCHLD: Send SIGCHLD to parent when child terminates
    int flags = SIGCHLD;
    
    pid_t child_pid = clone(child_function, 
                           (char *)child_stack + STACK_SIZE,  // Stack pointer
                           flags, 
                           (void *)"Hello from child process!");
    
    if (child_pid == -1) {
        perror("clone");
        free(child_stack);
        exit(1);
    }
    
    printf("Parent: Created child process with PID: %d\n", child_pid);
    
    // Show separate memory space
    int local_var = 50;
    printf("Parent: Memory address: %p\n", &child_stack);
    printf("Parent: Local variable address: %p, value: %d\n", &local_var, local_var);
    
    // Wait for the child process to complete
    int status;
    pid_t waited_pid = waitpid(child_pid, &status, 0);
    
    if (waited_pid == -1) {
        perror("waitpid");
    } else {
        if (WIFEXITED(status)) {
            printf("Parent: Child exited with status: %d\n", WEXITSTATUS(status));
        }
    }
    
    free(child_stack);
    printf("Parent: Child process completed\n");
    return 0;
}
