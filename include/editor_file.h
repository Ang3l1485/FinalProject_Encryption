#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

#include "io_backend.h"

#include <stddef.h>

int editor_file_save(
    const char *path,
    const unsigned char *text,
    size_t size,
    CeioIoMode mode
);

int editor_file_load(
    const char *path,
    unsigned char **text,
    size_t *size
);

/*
 * Compatibility wrappers kept for stage 2 tests and incremental integration.
 * The final editor should prefer editor_file_save/editor_file_load.
 */
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
