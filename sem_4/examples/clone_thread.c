#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define STACK_SIZE (1024 * 1024)

int f(void *a) {
	printf("PID: %d, TID: %d\n", getpid(), gettid());	
	return 0;
}

int main() {
	void *thread_stack = malloc(STACK_SIZE);
	if (thread_stack == NULL) {
		return 1;
	}

	int flags = SIGCHLD;

	pid_t tid = clone(f, (char*)thread_stack + STACK_SIZE, flags, NULL);
	if (tid == -1) {
		return 2;
	}
	
	printf("Parent thread: PID - %d, TID - %d\n", getpid(), gettid());
	printf("Created thread with TID: %d\n", tid);
	sleep(1);
	return 0;
}
