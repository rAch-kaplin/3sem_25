#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#define BASE 4096

void major_pf() {
    int fd = open("/dev/zero", O_RDONLY);
    if  (fd == -1) {
        perror("File read");
        return;
    }

    char *p = (char*)mmap(NULL, 1024*1024, PROT_READ, MAP_PRIVATE, fd, 0);

    char c;
    for (int i = 0; i < 1024*1024; i++)
        c = p[i];
    munmap(p, 1024*1024);
}

void function_to_measure() {
    size_t size  = 1024 * 1024;
    char *buf = (char*)malloc(size);

    for (int i = 0; i < size; i+=BASE) {
        buf[(rand())%(size-1)] = i + 1;
        // printf("i: %d\n", i/BASE);
    }

    return;
}

void get_rusage_stats(struct rusage *usage) {
    if (getrusage(RUSAGE_SELF, usage) == -1) {
        perror("getusage");
    }
    return;
}

void print_diff(struct rusage *before, struct rusage *after) {
    printf("Page faults: minor %ld, major %ld\n", 
            after->ru_minflt - before->ru_minflt,
            after->ru_majflt - before->ru_majflt);
    return;
}

void test_wrapper() {
    struct rusage before, after;

    get_rusage_stats(&before);
    // something to test_wrapper
    major_pf();
    get_rusage_stats(&after);

    print_diff(&before, &after);
}

int main() {
    test_wrapper();
}
