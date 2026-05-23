#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

#include "io_backend.h"

#include <stddef.h>

int editor_file_save(
    const char *path,
    const unsigned char *text,
    size_t size,
    CeioIoMode mode,
    const unsigned char *key,
    size_t key_size
);

int editor_file_load(
    const char *path,
    unsigned char **text,
    size_t *size,
    const unsigned char *key,
    size_t key_size
);

int save_ceio_file(
    const char *path,
    const unsigned char *text,
    size_t size
);

int load_ceio_file(
    const char *path,
    unsigned char **text,
    size_t *size
);

#endif