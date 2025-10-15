#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    int old = sigblock(sigmask(SIGUSR1));  // Block signal
    kill(getpid(), SIGUSR1);               // Send while blocked
    sleep(5);
    sigsetmask(old);                       // Restore old mask
    return 42;
}
