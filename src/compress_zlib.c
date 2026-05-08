#include "../include/compress_zlib.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int compress_buffer(
    const unsigned char *input,
    size_t input_size,
    unsigned char **output,
    size_t *output_size
) {
    uLongf bound = compressBound(input_size);

    *output = malloc(bound);

    if (*output == NULL) {
        return -1;
    }

    int result = compress(
        *output,
        &bound,
        input,
        input_size
    );

    if (result != Z_OK) {
        free(*output);
        return -1;
    }

    *output_size = bound;

    return 0;
}

int decompress_buffer(
    const unsigned char *input,
    size_t input_size,
    unsigned char **output,
    size_t expected_size
) {
    *output = malloc(expected_size);

    if (*output == NULL) {
        return -1;
    }

    uLongf final_size = expected_size;

    int result = uncompress(
        *output,
        &final_size,
        input,
        input_size
    );

    if (result != Z_OK) {
        free(*output);
        return -1;
    }

    return 0;
}