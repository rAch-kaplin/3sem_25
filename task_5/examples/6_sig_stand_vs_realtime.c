#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

volatile int standard_processed = 0;
volatile int realtime_processed = 0;

void standard_handler(int sig) {
    standard_processed++;
    printf("Standard: Processing signal %d (total processed: %d)\n", 
           sig, standard_processed);

    usleep(100000); // 100ms delay
}

void realtime_handler(int sig) {
    realtime_processed++;
    printf("Realtime: Processing signal %d (total processed: %d)\n", 
           sig, realtime_processed);

    usleep(100000); // 100ms delay
}

int main() {
    struct sigaction sa;
    pid_t pid = getpid();
    
    printf("Process PID: %d\n", pid);
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = standard_handler;
    sigaction(SIGUSR1, &sa, NULL);
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = realtime_handler;
    sigaction(SIGRTMIN, &sa, NULL);
    
    printf("Sending 10 signals of each type...\n");
    printf("Each handler will take 100ms to process\n\n");
    
    // Fork and have child send signals
    if (fork() == 0) {
        // Child process - send signals rapidly
        for (int i = 1; i <= 10; i++) {
            kill(pid, SIGUSR1);
            kill(pid, SIGRTMIN);
            usleep(10000); // 10ms between sends
        }
        exit(0);
    }
    
    // Parent waits for signals
    sleep(3);
    
    printf("\n=== RESULTS ===\n");
    printf("Standard signals, Processed: %d\n", 
           standard_processed);
    printf("Realtime signals, Processed: %d\n", 
           realtime_processed);
    
    return 0;
}
