#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main() {
	int pipefd[2] = {};
	pid_t pid = -1;
	char buffer[1024] = {};

	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	int fdl_dup = dup(pipefd[1]);
	fcntl(fdl_dup, F_SETFL, fcntl(fdl_dup, F_GETFL) | O_NONBLOCK);
	if (pid == 0) {
		close(pipefd[1]);
		while (read(pipefd[0], buffer, 1) > 0) {
			usleep(500);
		}
	} 
	else {
		int bytes = 0;
		close(pipefd[0]);
		while (1) {
			int res = write(fdl_dup, buffer, sizeof(buffer));
			if (res == -1) {
				if (errno == EAGAIN) {
					printf("Pipe full, %d bytes written\n", bytes);
					exit(2);
				}
				else {
					perror("Another error with pipe");
				}
			}
			else {
				bytes += res;
			}
		}
	}
}
