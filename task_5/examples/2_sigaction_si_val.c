#include <signal.h>
#include <unistd.h>
#include <stdio.h>


void handler(int sig, siginfo_t *info, void *ucontext) {
    printf("Transmitted: %d\n", info->si_value.sival_int);  // Read integer from sival
}

int main() {
    struct sigaction sa = {.sa_sigaction = handler, .sa_flags = SA_SIGINFO};
    sigaction(SIGUSR1, &sa, NULL);
    
    union sigval sv = {.sival_int = 42};  // Set integer in sival
    sigqueue(getpid(), SIGUSR1, sv);      // Send with data
    
    return 0;
}
