#include <signal.h>
#include <unistd.h>
#include <unistd.h>

void handler(int sig) {
    char c = '0' + (sig - SIGRTMIN);
    write(1, &c, 1);
}

int main() {
    struct sigaction sa = {.sa_handler = handler};
    sigaction(SIGRTMIN, &sa, NULL);
    sigaction(SIGRTMIN+1, &sa, NULL);
    sigaction(SIGRTMIN+3, &sa, NULL);
    int pid = getpid();
    if(fork() == 0) {
        kill(pid, SIGRTMIN);
        kill(pid, SIGRTMIN+1);
        kill(pid, SIGRTMIN+3);
        _exit(0);
    }
    pause();
}
