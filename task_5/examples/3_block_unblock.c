#include <signal.h>

int main() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, NULL);  // Block signal
    kill(getpid(), SIGINT);             // Send (queued)
    sleep(5);
    sigprocmask(SIG_UNBLOCK, &set, NULL); // Unblock - handler runs
    printf("Code after unblock\n");
    return 42;
}
