#ifndef CEIO_FORMAT_H
#define CEIO_FORMAT_H

#include <stddef.h>
#include <stdint.h>

#define CEIO_MAGIC "CEIO"
#define CEIO_VERSION 2
#define CEIO_IV_MAX 16

typedef struct {
    char magic[4];
    uint32_t version;

    uint32_t original_size;
    uint32_t compressed_size;
    uint32_t encrypted_size;

    uint32_t crc32;

    uint8_t crypto_algo;
    uint8_t iv_size;
    uint8_t iv[CEIO_IV_MAX];

} CeioHeader;

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
);

int ceio_format_parse(
    const unsigned char *file_data,
    size_t file_size,
    CeioHeader *out_header,
    const unsigned char **out_payload
);

#endif
