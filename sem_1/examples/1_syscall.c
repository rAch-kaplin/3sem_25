#include <unistd.h>
#include <sys/syscall.h>

// Direct system call wrapper (Linux-specific)
void simple_write(const char *msg, int length) {
    syscall(SYS_write, 1, msg, length);
}

int main() {
    simple_write("Direct syscall!\n", 16);
    return 0;
}
