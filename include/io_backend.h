#ifndef IO_BACKEND_H
#define IO_BACKEND_H

#include <stddef.h>

int write_file(
    const char *path,
    const unsigned char *data,
    size_t size
);

int read_file(
    const char *path,
    unsigned char **data,
    size_t *size
);

#endif