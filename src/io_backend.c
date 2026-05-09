#include "io_backend.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int write_all(int fd, const unsigned char *data, size_t size) {
    size_t total_written = 0;

    while (total_written < size) {
        ssize_t chunk = write(fd, data + total_written, size - total_written);
        if (chunk <= 0) {
            return -1;
        }
        total_written += (size_t)chunk;
    }

    return 0;
}

static int io_backend_write_with_write(
    const char *path,
    const unsigned char *data,
    size_t size
) {
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }

    int result = write_all(fd, data, size);
    if (close(fd) != 0) {
        return -1;
    }
    return result;
}

static int io_backend_write_with_mmap(
    const char *path,
    const unsigned char *data,
    size_t size
) {
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }

    if (size == 0) {
        int close_result = close(fd);
        return close_result == 0 ? 0 : -1;
    }

    if (ftruncate(fd, (off_t)size) != 0) {
        close(fd);
        return -1;
    }

    void *mapped = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return -1;
    }

    memcpy(mapped, data, size);

    int result = 0;
    if (msync(mapped, size, MS_SYNC) != 0) {
        result = -1;
    }
    if (munmap(mapped, size) != 0) {
        result = -1;
    }
    if (close(fd) != 0) {
        result = -1;
    }

    return result;
}

int io_backend_write_file(
    const char *path,
    const unsigned char *data,
    size_t size,
    CeioIoMode mode
) {
    if (path == NULL || (data == NULL && size > 0)) {
        return -1;
    }

    if (mode == CEIO_IO_MMAP) {
        return io_backend_write_with_mmap(path, data, size);
    }
    return io_backend_write_with_write(path, data, size);
}

int io_backend_read_file(
    const char *path,
    unsigned char **data,
    size_t *size
) {
    struct stat st;

    if (path == NULL || data == NULL || size == NULL) {
        return -1;
    }
    if (stat(path, &st) != 0) {
        return -1;
    }

    *size = (size_t)st.st_size;
    *data = NULL;

    if (*size == 0) {
        return 0;
    }

    unsigned char *buffer = malloc(*size);
    if (buffer == NULL) {
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        free(buffer);
        return -1;
    }

    size_t total_read = 0;
    while (total_read < *size) {
        ssize_t chunk = read(fd, buffer + total_read, *size - total_read);
        if (chunk <= 0) {
            close(fd);
            free(buffer);
            return -1;
        }
        total_read += (size_t)chunk;
    }

    if (close(fd) != 0) {
        free(buffer);
        return -1;
    }

    *data = buffer;
    return 0;
}

const char *ceio_io_mode_name(CeioIoMode mode) {
    if (mode == CEIO_IO_MMAP) {
        return "mmap";
    }
    return "write";
}
