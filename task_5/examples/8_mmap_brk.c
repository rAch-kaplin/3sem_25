#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    void *start = sbrk(0);
    printf("Начальный brk: %p\n", start);
    
    // Маленькое выделение - использует brk
    void *small = malloc(1024);
    void *after_small = sbrk(0);
    printf("После small malloc: brk = %p (изменение: %ld байт)\n", 
           after_small, (char*)after_small - (char*)start);
    
    // Большое выделение - переключится на mmap
    void *large = malloc(128 * 1024);  // 128KB обычно превышает MMAP_THRESHOLD
    void *after_large = sbrk(0);
    printf("После large malloc: brk = %p (изменение: %ld байт)\n",
           after_large, (char*)after_large - (char*)after_small);
    
    free(small);
    free(large);
    return 0;
}
