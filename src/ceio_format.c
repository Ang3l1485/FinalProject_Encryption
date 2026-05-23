#include "ceio_format.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int ceio_format_build(
    uint32_t original_size,
    uint32_t compressed_size,
    const unsigned char *encrypted,
    uint32_t encrypted_size,
    const unsigned char *iv,
    uint8_t iv_size,
    uint32_t crc32_value,
    unsigned char **out_buffer,
    size_t *out_size
) {
    if (encrypted == NULL || out_buffer == NULL || out_size == NULL || iv == NULL) {
        return -1;
    }

    if (iv_size > CEIO_IV_MAX) {
        return -1;
    }

    size_t total_size = sizeof(CeioHeader) + encrypted_size;
    unsigned char *buffer = malloc(total_size);
    if (buffer == NULL) {
        return -1;
    }

    CeioHeader header;

    memcpy(header.magic, CEIO_MAGIC, 4);
    header.version = CEIO_VERSION;

    header.original_size = original_size;
    header.compressed_size = compressed_size;
    header.encrypted_size = encrypted_size;

    header.crc32 = crc32_value;

    header.crypto_algo = 1;
    header.iv_size = iv_size;

    memset(header.iv, 0, CEIO_IV_MAX);
    memcpy(header.iv, iv, iv_size);

    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), encrypted, encrypted_size);

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

    if (memcmp(out_header->magic, CEIO_MAGIC, 4) != 0) {
        return -1;
    }

    if (out_header->version != CEIO_VERSION) {
        return -1;
    }

    if (out_header->iv_size > CEIO_IV_MAX) {
        return -1;
    }

    size_t expected_size = sizeof(CeioHeader) + out_header->encrypted_size;
    if (expected_size != file_size) {
        return -1;
    }

    *out_payload = file_data + sizeof(CeioHeader);

    return 0;
}