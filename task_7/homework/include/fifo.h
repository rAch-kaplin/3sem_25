#ifndef FIFO_H
#define FIFO_H

void cleanup_fifo(const char *fifo_path);
int file_exists(const char *path);
int create_fifo_with_path(const char *fifo_path);

#endif // FIFO_H
