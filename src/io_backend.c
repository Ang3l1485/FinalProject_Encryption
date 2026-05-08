#include "../include/io_backend.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

int write_file(
    const char *path,
    const unsigned char *data,
    size_t size
) {
    int fd = open(
        path,
        O_CREAT | O_WRONLY | O_TRUNC,
        0644
    );

    if (fd < 0) {
        return -1;
    }

    ssize_t written = write(
        fd,
        data,
        size
    );

    close(fd);

    return written == (ssize_t)size
        ? 0
        : -1;
}

int read_file(
    const char *path,
    unsigned char **data,
    size_t *size
) {
    struct stat st;

    if (stat(path, &st) < 0) {
        return -1;
    }

    *size = st.st_size;

    *data = malloc(*size);

    if (*data == NULL) {
        return -1;
    }

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        free(*data);
        return -1;
    }

    ssize_t bytes = read(
        fd,
        *data,
        *size
    );

    close(fd);

    return bytes == (ssize_t)(*size)
        ? 0
        : -1;
}