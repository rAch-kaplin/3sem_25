#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int pipefd[2];
    pid_t pid;
    
    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }
    
    // Fork a child process
    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) { // Child process
        close(pipefd[1]); // Close write end of pipe
        char buf[4096];
        // Redirect stdin to read from the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        read(0, buf, sizeof(buf));
	    write(1, buf, strlen(buf));
        return 1;
        
    } else { // Parent process
        close(pipefd[0]); // Close read end of pipe
        
        // Redirect stdout to write to the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Parent's output will go to child's input
        printf("Hello from parent process!\n");
        printf("This text is sent through the pipe\n");
        printf("To the child process for processing\n");
        
        // Important: close stdout to send EOF to child
        fclose(stdout);
        
        // Wait for child to finish
        wait(NULL);
    }
    
    return 0;
}
