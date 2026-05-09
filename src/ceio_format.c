#include "ceio_format.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int ceio_format_build(
    size_t original_size,
    const unsigned char *compressed,
    size_t compressed_size,
    uint32_t crc32_value,
    unsigned char **out_buffer,
    size_t *out_size
) {
    if (compressed == NULL || out_buffer == NULL || out_size == NULL) {
        return -1;
    }
    if (original_size > UINT32_MAX || compressed_size > UINT32_MAX) {
        return -1;
    }

    size_t total_size = sizeof(CeioHeader) + compressed_size;
    unsigned char *buffer = malloc(total_size);
    if (buffer == NULL) {
        return -1;
    }

    CeioHeader header;
    memcpy(header.magic, "CEIO", sizeof(header.magic));
    header.version = 1u;
    header.original_size = (uint32_t)original_size;
    header.compressed_size = (uint32_t)compressed_size;
    header.crc32 = crc32_value;

    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), compressed, compressed_size);

    *out_buffer = buffer;
    *out_size = total_size;
    return 0;
}

int ceio_format_parse(
    const unsigned char *file_data,
    size_t file_size,
    CeioHeader *out_header,
    const unsigned char **out_payload
) {
    if (file_data == NULL || out_header == NULL || out_payload == NULL) {
        return -1;
    }
    if (file_size < sizeof(CeioHeader)) {
        return -1;
    }

    memcpy(out_header, file_data, sizeof(*out_header));
    if (memcmp(out_header->magic, "CEIO", sizeof(out_header->magic)) != 0) {
        return -1;
    }
    if (out_header->version != 1u) {
        return -1;
    }

    size_t payload_size = file_size - sizeof(CeioHeader);
    if (payload_size != (size_t)out_header->compressed_size) {
        return -1;
    }

    *out_payload = file_data + sizeof(CeioHeader);
    return 0;
}
