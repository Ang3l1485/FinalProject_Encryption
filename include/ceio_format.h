#ifndef CEIO_FORMAT_H
#define CEIO_FORMAT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t original_size;
    uint32_t compressed_size;
    uint32_t crc32;
} CeioHeader;

int ceio_format_build(
    size_t original_size,
    const unsigned char *compressed,
    size_t compressed_size,
    uint32_t crc32_value,
    unsigned char **out_buffer,
    size_t *out_size
);

int ceio_format_parse(
    const unsigned char *file_data,
    size_t file_size,
    CeioHeader *out_header,
    const unsigned char **out_payload
);

#endif
