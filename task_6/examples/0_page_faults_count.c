#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void memory_intensive_function() {
    printf("Starting memory-intensive function...\n");
    
    // Allocate and access memory to generate page faults
    const size_t size = 100 * 1024 * 1024; // 100 MB
    char *buffer = malloc(size);
    int base = 1;
    int limit = size;
    
    if (buffer) {
        buffer[0] = 1;
        // Touch every page to generate page faults
        for (size_t i = 0; i < size; i += base) {
            buffer[i] = (char)(i % 250);
        }
        
        free(buffer);
    }
    
    printf("Memory-intensive function completed.\n");
}


void get_rusage_stats(struct rusage *usage) {
    if (getrusage(RUSAGE_SELF, usage) == -1) {
        perror("getrusage");
    }
}

void print_rusage_diff(struct rusage *before, struct rusage *after) {
    printf("Page faults: minor = %ld, major = %ld\n",
           after->ru_minflt - before->ru_minflt,
           after->ru_majflt - before->ru_majflt);
    
    printf("CPU time: user = %.6f sec, system = %.6f sec\n",
           (after->ru_utime.tv_sec - before->ru_utime.tv_sec) +
           (after->ru_utime.tv_usec - before->ru_utime.tv_usec) / 1000000.0,
           (after->ru_stime.tv_sec - before->ru_stime.tv_sec) +
           (after->ru_stime.tv_usec - before->ru_stime.tv_usec) / 1000000.0);
}

void test_with_rusage() {
    printf("\n=== Testing with getrusage method ===\n");
    
    struct rusage before, after;
    
    get_rusage_stats(&before);
    memory_intensive_function();
    get_rusage_stats(&after);
    
    print_rusage_diff(&before, &after);
}

int main() {
	test_with_rusage();
	return 0;
}
