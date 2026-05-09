#ifndef COMPRESS_ZLIB_H
#define COMPRESS_ZLIB_H

#include <stddef.h>

int compress_buffer(
    const unsigned char *input,
    size_t input_size,
    unsigned char **output,
    size_t *output_size
);

int decompress_buffer(
    const unsigned char *input,
    size_t input_size,
    unsigned char **output,
    size_t expected_size
);

#endif