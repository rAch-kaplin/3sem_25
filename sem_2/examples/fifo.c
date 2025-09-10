#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

int main() {
	int fd;
	char *fifo_path = "myfifo";
	mkfifo(fifo_path, 0666);
	char buf[4096];

	fd = open(fifo_path, O_WRONLY);
	while (1) {
		scanf("%s", buf);
		write(fd, buf, strlen(buf) + 1);
	}

	return 0;
}
