#include "editor_file.h"

#include "ceio_format.h"
#include "compress_zlib.h"

#include <stdlib.h>
#include <zlib.h>

int editor_file_save(
    const char *path,
    const unsigned char *text,
    size_t size,
    CeioIoMode mode
) {
    unsigned char *compressed = NULL;
    size_t compressed_size = 0;
    unsigned char *file_buffer = NULL;
    size_t file_size = 0;

    if (path == NULL || (text == NULL && size > 0)) {
        return -1;
    }

    if (compress_buffer(text, size, &compressed, &compressed_size) != 0) {
        return -1;
    }

    if (ceio_format_build(
        size,
        compressed,
        compressed_size,
        crc32(0L, text, size),
        &file_buffer,
        &file_size
    ) != 0) {
        free(compressed);
        return -1;
    }

    /* file_buffer is the serialized .ceio payload and is released here. */
    int result = io_backend_write_file(path, file_buffer, file_size, mode);
    free(file_buffer);
    free(compressed);
    return result;
}

int editor_file_load(
    const char *path,
    unsigned char **text,
    size_t *size
) {
    unsigned char *file_buffer = NULL;
    size_t file_size = 0;
    CeioHeader header;
    const unsigned char *payload = NULL;
    unsigned char *decompressed = NULL;

    if (io_backend_read_file(path, &file_buffer, &file_size) != 0) {
        return -1;
    }
    if (ceio_format_parse(file_buffer, file_size, &header, &payload) != 0) {
        free(file_buffer);
        return -1;
    }
    if (decompress_buffer(
        payload,
        header.compressed_size,
        &decompressed,
        header.original_size
    ) != 0) {
        free(file_buffer);
        return -1;
    }
    if (crc32(0L, decompressed, header.original_size) != header.crc32) {
        free(decompressed);
        free(file_buffer);
        return -1;
    }

    *text = decompressed;
    *size = header.original_size;
    free(file_buffer);
    return 0;
}

int save_ceio_file(
    const char *path,
    const unsigned char *text,
    size_t size
) {
    return editor_file_save(path, text, size, CEIO_IO_WRITE);
}

int load_ceio_file(
    const char *path,
    unsigned char **text,
    size_t *size
) {
    return editor_file_load(path, text, size);
}
