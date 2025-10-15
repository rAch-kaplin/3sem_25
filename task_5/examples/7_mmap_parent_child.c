#include <sys/mman.h>
#include <unistd.h>

int main() {
    int *mem = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, 
                   MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    if (fork() == 0) {  // Child
        *mem = 42;
    } else {           // Parent  
        sleep(1);
        return *mem;   // Returns 42
    }
}
