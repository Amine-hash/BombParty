#include "tubes.h"

int create_named_pipe(const char *path) {
    if (mkfifo(path, 0666) == -1) {
        perror("mkfifo");
        return -1;
    }
    return 0;
}

int open_named_pipe(const char *path, int flags) {
    int fd;
    do{
    fd = open(path, flags);
    if (fd == -1 && errno != EINTR) {
        perror("open");
        return -1;
    }
    }while(errno == EINTR);
    return fd;
}

ssize_t write_to_pipe(int fd, const void *buf, size_t count) {
    ssize_t bytes_written = write(fd, buf, count);
    if (bytes_written == -1 && errno != EINTR) {
        perror("write");
        return -1;
    }
    return bytes_written;
}

ssize_t read_from_pipe(int fd, void *buf, size_t count) {
    ssize_t bytes_read = read(fd, buf, count);
    if (bytes_read == -1 && errno != EAGAIN && errno != EINTR) {
        perror("read");
        return -1;
    }
    return bytes_read;
}

void close_pipe(int fd) {
    if (close(fd) == -1) {
        perror("close");
    }
}
