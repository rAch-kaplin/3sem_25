#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#define _GNU_SOURCE  // For execvpe

void run_execl() {
    printf("1. Using execl():\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        execl("/bin/ls", "ls", "-l", "-t", "-r", (char *)NULL);
        perror("execl failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_execlp() {
    printf("2. Using execlp():\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        execlp("ls", "ls", "-l", "-t", "-r", (char *)NULL);
        perror("execlp failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_execle() {
    printf("3. Using execle():\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        char *envp[] = { NULL };
        execle("/bin/ls", "ls", "-l", "-t", "-r", (char *)NULL, envp);
        perror("execle failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_execv() {
    printf("4. Using execv():\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        char *args[] = {"ls", "-l", "-t", "-r", NULL};
        execv("/bin/ls", args);
        perror("execv failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_execvp() {
    printf("5. Using execvp():\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        char *args[] = {"ls", "-l", "-t", "-r", NULL};
        execvp("ls", args);
        perror("execvp failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_execvpe() {
    printf("6. Using execvpe() (Linux-specific):\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        char *args[] = {"ls", "-l", "-t", "-r", NULL};
        char *envp[] = { NULL };
        execvpe("ls", args, envp);
        perror("execvpe failed");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

void run_combined_flags() {
    printf("7. Using combined flags (-ltr):\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        char *args[] = {"ls", "-ltr", NULL};
        execvp("ls", args);
        perror("execvp failed with combined flags");
        exit(1);
    }
    wait(NULL);
    printf("\n");
}

int main() {
    printf("Demonstrating exec* calls for 'ls -ltr':\n");
    printf("=========================================\n\n");
    
    run_execl();
    run_execlp();
    run_execle();
    run_execv();
    run_execvp();
    run_execvpe();
    run_combined_flags();
    
    printf("All exec* variants demonstrated!\n");
    return 0;
}
