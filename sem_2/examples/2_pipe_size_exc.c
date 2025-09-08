#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[1024];
    int bytes_written = 0;
    
    // Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {  // Child process - reader
        close(pipefd[1]);  // Close write end
        
        // Slow reader - read one byte at a time with delay
        sleep(2);  // Give writer time to fill the buffer
        
        int bytes_read = 0;
        while (read(pipefd[0], buffer, 1) > 0) {
            bytes_read++;
            usleep(1000);  // 1ms delay per byte
        }
        
        printf("Child read %d bytes\n", bytes_read);
        close(pipefd[0]);
        
    } else {  // Parent process - writer
        close(pipefd[0]);  // Close read end
        
        // Fast writer - try to write as much as possible
        while (1) {
            int result = write(pipefd[1], "A", 1);
            if (result == -1) {
                if (errno == EAGAIN) {
                    printf("Pipe buffer full (EAGAIN) after %d bytes\n", bytes_written);
                    break;
                } else {
                    perror("write");
                    break;
                }
            }
            bytes_written += result;
            
            if (bytes_written % 1024 == 0) {
                printf("Written %d bytes so far...\n", bytes_written);
            }
        }
        
        close(pipefd[1]);
        printf("Total bytes written before blocking: %d\n", bytes_written);
    }
    
    return 0;
}
