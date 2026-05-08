#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

#include <stddef.h>

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