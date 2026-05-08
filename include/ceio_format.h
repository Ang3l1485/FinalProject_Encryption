#ifndef CEIO_FORMAT_H
#define CEIO_FORMAT_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t original_size;
    uint32_t compressed_size;
    uint32_t crc32;
} CeioHeader;

#endif