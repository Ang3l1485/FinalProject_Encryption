#ifndef IO_BACKEND_H
#define IO_BACKEND_H

#include <stddef.h>

typedef enum {
    CEIO_IO_WRITE = 0,
    CEIO_IO_MMAP = 1
} CeioIoMode;

int io_backend_write_file(
    const char *path,
    const unsigned char *data,
    size_t size,
    CeioIoMode mode
);

int io_backend_read_file(
    const char *path,
    unsigned char **data,
    size_t *size
);

const char *ceio_io_mode_name(CeioIoMode mode);

#endif
