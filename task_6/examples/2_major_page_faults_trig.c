#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/resource.h>
#endif

int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDONLY);
  struct stat stats;
  fstat(fd, &stats);
  posix_fadvise(fd, 0, stats.st_size, POSIX_FADV_DONTNEED);
  char * map = (char *) mmap(NULL, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("Failed to mmap");
    return 1;
  }
  
  int result = 0;
  int i;
  
  // Get initial page fault statistics
  #ifdef __linux__
  struct rusage usage_before, usage_after;
  getrusage(RUSAGE_SELF, &usage_before);
  #endif
  
  for (i = 0; i < stats.st_size; i++) {
    result += map[i];
  }
  
  // Get page fault statistics after main work
  #ifdef __linux__
  getrusage(RUSAGE_SELF, &usage_after);
  
  // Calculate major page faults during main work
  long major_faults_during_work = usage_after.ru_majflt - usage_before.ru_majflt;
  printf("Major page faults during main work: %ld\n", major_faults_during_work);
  #else
  printf("Page fault counting not available on this platform\n");
  #endif
  
  munmap(map, stats.st_size);
  
  exit(result);
}
