#include "editor_file.h"

#include "ceio_format.h"
#include "compress_zlib.h"
#include "crypto_ceio.h"

#include <stdlib.h>
#include <zlib.h>

int editor_file_save(
    const char *path,
    const unsigned char *text,
    size_t size,
    CeioIoMode mode,
    const unsigned char *key,
    size_t key_size
) {
    unsigned char *compressed = NULL;
    size_t compressed_size = 0;

    unsigned char *encrypted = NULL;
    size_t encrypted_size = 0;

    unsigned char iv[CRYPTO_CEIO_IV_SIZE];

    unsigned char *file_buffer = NULL;
    size_t file_size = 0;

    if (path == NULL || (text == NULL && size > 0)) {
        return -1;
    }

    if (compress_buffer(text, size, &compressed, &compressed_size) != 0) {
        return -1;
    }

    if (crypto_generate_iv(iv) != 0) {
        free(compressed);
        return -1;
    }

    if (crypto_encrypt_buffer(
            compressed,
            compressed_size,
            key,
            key_size,
            iv,
            &encrypted,
            &encrypted_size
        ) != 0) {
        free(compressed);
        return -1;
    }

    if (ceio_format_build(
            (uint32_t)size,
            (uint32_t)compressed_size,
            encrypted,
            (uint32_t)encrypted_size,
            iv,
            CRYPTO_CEIO_IV_SIZE,
            crc32(0L, text, size),
            &file_buffer,
            &file_size
        ) != 0) {
        free(compressed);
        free(encrypted);
        return -1;
    }

    int result = io_backend_write_file(path, file_buffer, file_size, mode);

    free(file_buffer);
    free(compressed);
    free(encrypted);

    return result;
}

int editor_file_load(
    const char *path,
    unsigned char **text,
    size_t *size,
    const unsigned char *key,
    size_t key_size
) {
    unsigned char *file_buffer = NULL;
    size_t file_size = 0;

    CeioHeader header;
    const unsigned char *payload = NULL;

    unsigned char *decrypted = NULL;
    size_t decrypted_size = 0;

    unsigned char *decompressed = NULL;

    if (io_backend_read_file(path, &file_buffer, &file_size) != 0) {
        return -1;
    }

    if (ceio_format_parse(file_buffer, file_size, &header, &payload) != 0) {
        free(file_buffer);
        return -1;
    }

    if (crypto_decrypt_buffer(
            payload,
            header.encrypted_size,
            key,
            key_size,
            header.iv,
            &decrypted,
            &decrypted_size
        ) != 0) {
        free(file_buffer);
        return -1;
    }

    if (decompress_buffer(
            decrypted,
            header.compressed_size,
            &decompressed,
            header.original_size
        ) != 0) {
        free(decrypted);
        free(file_buffer);
        return -1;
    }

    if (crc32(0L, decompressed, header.original_size) != header.crc32) {
        free(decompressed);
        free(decrypted);
        free(file_buffer);
        return -1;
    }

    *text = decompressed;
    *size = header.original_size;

    free(decrypted);
    free(file_buffer);

    return 0;
}

static const unsigned char DEFAULT_KEY[16] = "default_test_key";

int save_ceio_file(
    const char *path,
    const unsigned char *text,
    size_t size
) {
    return editor_file_save(
        path,
        text,
        size,
        CEIO_IO_WRITE,
        DEFAULT_KEY,
        sizeof(DEFAULT_KEY)
    );
}

int load_ceio_file(
    const char *path,
    unsigned char **text,
    size_t *size
) {
    return editor_file_load(
        path,
        text,
        size,
        DEFAULT_KEY,
        sizeof(DEFAULT_KEY)
    );
}