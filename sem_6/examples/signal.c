#include <stdio.h>
#include <signal.h>

void handler(int sig_no) {
    printf("AHHAHAHAAH\n");
    return;
}
int main() {
    signal(SIGINT, handler);
    while (1);
    return 0;
}
