#include "../include/editor_file.h"
#include "../include/compress_zlib.h"
#include "../include/io_backend.h"
#include "../include/ceio_format.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int save_ceio_file(
    const char *path,
    const unsigned char *text,
    size_t size
) {
    unsigned char *compressed = NULL;
    size_t compressed_size = 0;

    if (compress_buffer(
        text,
        size,
        &compressed,
        &compressed_size
    ) != 0) {
        return -1;
    }

    CeioHeader header = {
        .magic = {'C', 'E', 'I', 'O'},
        .version = 1,
        .original_size = size,
        .compressed_size = compressed_size,
        .crc32 = crc32(0L, text, size)
    };

    size_t final_size =
        sizeof(CeioHeader)
        + compressed_size;

    unsigned char *final_buffer =
        malloc(final_size);

    if (final_buffer == NULL) {
        free(compressed);
        return -1;
    }

    memcpy(
        final_buffer,
        &header,
        sizeof(CeioHeader)
    );

    memcpy(
        final_buffer + sizeof(CeioHeader),
        compressed,
        compressed_size
    );

    int result = write_file(
        path,
        final_buffer,
        final_size
    );

    free(compressed);
    free(final_buffer);

    return result;
}

int load_ceio_file(
    const char *path,
    unsigned char **text,
    size_t *size
) {
    unsigned char *buffer = NULL;
    size_t file_size = 0;

    if (read_file(
        path,
        &buffer,
        &file_size
    ) != 0) {
        return -1;
    }

    CeioHeader header;

    memcpy(
        &header,
        buffer,
        sizeof(CeioHeader)
    );

    if (memcmp(
        header.magic,
        "CEIO",
        4
    ) != 0) {
        free(buffer);
        return -1;
    }

    unsigned char *compressed =
        buffer + sizeof(CeioHeader);

    if (decompress_buffer(
        compressed,
        header.compressed_size,
        text,
        header.original_size
    ) != 0) {
        free(buffer);
        return -1;
    }

    *size = header.original_size;

    free(buffer);

    return 0;
}