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
    static const unsigned char empty_input = '\0';
    uLongf bound = compressBound(input_size);
    const unsigned char *source = input_size == 0 ? &empty_input : input;

    if (output == NULL || output_size == NULL || (input == NULL && input_size > 0)) {
        return -1;
    }

    *output = malloc(bound);

    if (*output == NULL) {
        return -1;
    }

    int result = compress(
        *output,
        &bound,
        source,
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
    static const unsigned char empty_input = '\0';
    unsigned char *buffer = malloc(expected_size > 0 ? expected_size : 1u);
    const unsigned char *source = input_size == 0 ? &empty_input : input;

    if (output == NULL || (input == NULL && input_size > 0)) {
        free(buffer);
        return -1;
    }

    if (buffer == NULL) {
        return -1;
    }

    uLongf final_size = expected_size;

    int result = uncompress(
        buffer,
        &final_size,
        source,
        input_size
    );

    if (result != Z_OK) {
        free(buffer);
        return -1;
    }

    *output = buffer;
    return 0;
}
