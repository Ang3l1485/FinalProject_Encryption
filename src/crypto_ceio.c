
#include "crypto_ceio.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define _DEFAULT_SOURCE

#include <zlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <unistd.h>

#define XTEA_DELTA 0x9E3779B9u
#define XTEA_ROUNDS 32u

static uint32_t load_u32_le(const unsigned char *bytes) {
    return ((uint32_t)bytes[0]) |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void store_u32_le(unsigned char *bytes, uint32_t value) {
    bytes[0] = (unsigned char)(value & 0xffu);
    bytes[1] = (unsigned char)((value >> 8) & 0xffu);
    bytes[2] = (unsigned char)((value >> 16) & 0xffu);
    bytes[3] = (unsigned char)((value >> 24) & 0xffu);
}

static void xtea_encrypt_block(unsigned char block[CRYPTO_CEIO_BLOCK_SIZE], const uint32_t key[4]) {
    uint32_t v0 = load_u32_le(block);
    uint32_t v1 = load_u32_le(block + 4);
    uint32_t sum = 0;

    for (uint32_t round = 0; round < XTEA_ROUNDS; round++) {
        v0 += ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3u]));
        sum += XTEA_DELTA;
        v1 += ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3u]));
    }

    store_u32_le(block, v0);
    store_u32_le(block + 4, v1);
}

static void xtea_decrypt_block(unsigned char block[CRYPTO_CEIO_BLOCK_SIZE], const uint32_t key[4]) {
    uint32_t v0 = load_u32_le(block);
    uint32_t v1 = load_u32_le(block + 4);
    uint32_t sum = XTEA_DELTA * XTEA_ROUNDS;

    for (uint32_t round = 0; round < XTEA_ROUNDS; round++) {
        v1 -= ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3u]));
        sum -= XTEA_DELTA;
        v0 -= ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3u]));
    }

    store_u32_le(block, v0);
    store_u32_le(block + 4, v1);
}

static size_t pkcs7_padded_size(size_t input_size) {
    size_t remainder = input_size % CRYPTO_CEIO_BLOCK_SIZE;
    size_t padding = CRYPTO_CEIO_BLOCK_SIZE - remainder;

    if (padding == 0) {
        padding = CRYPTO_CEIO_BLOCK_SIZE;
    }

    return input_size + padding;
}

static int derive_xtea_key(
    const unsigned char *key_material,
    size_t key_size,
    uint32_t out_key[CRYPTO_CEIO_KEY_WORDS]
) {
    static const uint32_t seeds[CRYPTO_CEIO_KEY_WORDS] = {
        0x13579BDFu, 0x2468ACE0u, 0xA5A5A5A5u, 0x5A5A5A5Au
    };

    if (key_material == NULL || out_key == NULL || key_size == 0) {
        return -1;
    }
    if (key_size > UINT_MAX) {
        return -1;
    }

    for (size_t i = 0; i < CRYPTO_CEIO_KEY_WORDS; i++) {
        uint32_t crc = crc32(seeds[i], key_material, (uInt)key_size);
        uint32_t mix = (uint32_t)key_size * 0x9E3779B9u;
        out_key[i] = crc ^ mix ^ (seeds[i] << (unsigned int)i);
    }

    return 0;
}

static int apply_padding_and_prefix_crc(
    const unsigned char *plaintext,
    size_t plaintext_size,
    unsigned char **buffer,
    size_t *buffer_size
) {
    size_t payload_size = sizeof(uint32_t) + plaintext_size;
    const unsigned char *crc_source = plaintext_size == 0 ? (const unsigned char *)"" : plaintext;

    if (buffer == NULL || buffer_size == NULL) {
        return -1;
    }
    if (plaintext == NULL && plaintext_size > 0) {
        return -1;
    }
    if (payload_size < plaintext_size) {
        return -1;
    }
    if (plaintext_size > UINT_MAX) {
        return -1;
    }

    uint32_t crc = crc32(0L, crc_source, (uInt)plaintext_size);

    size_t padded_size = pkcs7_padded_size(payload_size);
    unsigned char *tmp = malloc(padded_size);
    if (tmp == NULL) {
        return -1;
    }

    store_u32_le(tmp, crc);
    if (plaintext_size > 0) {
        memcpy(tmp + sizeof(uint32_t), plaintext, plaintext_size);
    }

    unsigned char pad_value = (unsigned char)(padded_size - payload_size);
    memset(tmp + payload_size, pad_value, padded_size - payload_size);

    *buffer = tmp;
    *buffer_size = padded_size;
    return 0;
}

static int remove_padding_and_validate_crc(
    unsigned char *buffer,
    size_t buffer_size,
    unsigned char **plaintext,
    size_t *plaintext_size
) {
    if (buffer == NULL || plaintext == NULL || plaintext_size == NULL) {
        return -1;
    }
    if (buffer_size < CRYPTO_CEIO_BLOCK_SIZE || (buffer_size % CRYPTO_CEIO_BLOCK_SIZE) != 0) {
        return -1;
    }

    unsigned char pad_value = buffer[buffer_size - 1];
    if (pad_value == 0 || pad_value > CRYPTO_CEIO_BLOCK_SIZE) {
        return -1;
    }

    for (size_t i = 0; i < pad_value; i++) {
        if (buffer[buffer_size - 1 - i] != pad_value) {
            return -1;
        }
    }

    size_t payload_size = buffer_size - pad_value;
    if (payload_size < sizeof(uint32_t)) {
        return -1;
    }

    size_t message_size = payload_size - sizeof(uint32_t);
    uint32_t expected_crc = load_u32_le(buffer);
    if (message_size > UINT_MAX) {
        return -1;
    }
    uint32_t actual_crc = crc32(0L, buffer + sizeof(uint32_t), (uInt)message_size);

    if (expected_crc != actual_crc) {
        return -1;
    }

    unsigned char *out = malloc(message_size > 0 ? message_size : 1u);
    if (out == NULL) {
        return -1;
    }

    if (message_size > 0) {
        memcpy(out, buffer + sizeof(uint32_t), message_size);
    }

    *plaintext = out;
    *plaintext_size = message_size;
    return 0;
}

void secure_zero_memory(void *ptr, size_t size) {
    if (ptr == NULL || size == 0) {
        return;
    }

    explicit_bzero(ptr, size);
}

int crypto_try_lock_memory(void *ptr, size_t size) {
    if (ptr == NULL || size == 0) {
        return -1;
    }

    return mlock(ptr, size) == 0 ? 0 : -1;
}

void crypto_try_unlock_memory(void *ptr, size_t size) {
    if (ptr != NULL && size > 0) {
        (void)munlock(ptr, size);
    }
}

int crypto_secure_alloc_key_copy(
    const unsigned char *source,
    size_t size,
    unsigned char **locked_copy
) {
    if (source == NULL || locked_copy == NULL || size == 0) {
        return -1;
    }

    unsigned char *copy = malloc(size);
    if (copy == NULL) {
        return -1;
    }

    memcpy(copy, source, size);
    (void)crypto_try_lock_memory(copy, size);

    *locked_copy = copy;
    return 0;
}

void crypto_secure_free_key(unsigned char *key, size_t size) {
    if (key == NULL) {
        return;
    }

    secure_zero_memory(key, size);
    crypto_try_unlock_memory(key, size);
    free(key);
}

int crypto_generate_iv(unsigned char iv[CRYPTO_CEIO_IV_SIZE]) {
    if (iv == NULL) {
        return -1;
    }

    ssize_t bytes_read = getrandom(iv, CRYPTO_CEIO_IV_SIZE, 0);
    if (bytes_read == (ssize_t)CRYPTO_CEIO_IV_SIZE) {
        return 0;
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    size_t total = 0;
    while (total < CRYPTO_CEIO_IV_SIZE) {
        ssize_t chunk = read(fd, iv + total, CRYPTO_CEIO_IV_SIZE - total);
        if (chunk <= 0) {
            close(fd);
            return -1;
        }
        total += (size_t)chunk;
    }

    if (close(fd) != 0) {
        return -1;
    }
    return 0;
}

int crypto_encrypt_buffer(
    const unsigned char *plaintext,
    size_t plaintext_size,
    const unsigned char *key_material,
    size_t key_size,
    const unsigned char iv[CRYPTO_CEIO_IV_SIZE],
    unsigned char **ciphertext,
    size_t *ciphertext_size
) {
    uint32_t key_words[CRYPTO_CEIO_KEY_WORDS] = {0};
    unsigned char previous_block[CRYPTO_CEIO_BLOCK_SIZE];
    unsigned char *working = NULL;
    size_t working_size = 0;

    if (ciphertext == NULL || ciphertext_size == NULL || key_material == NULL || iv == NULL) {
        return -1;
    }
    if (plaintext == NULL && plaintext_size > 0) {
        return -1;
    }
    if (derive_xtea_key(key_material, key_size, key_words) != 0) {
        return -1;
    }
    if (apply_padding_and_prefix_crc(plaintext, plaintext_size, &working, &working_size) != 0) {
        secure_zero_memory(key_words, sizeof(key_words));
        return -1;
    }

    memcpy(previous_block, iv, sizeof(previous_block));
    for (size_t offset = 0; offset < working_size; offset += CRYPTO_CEIO_BLOCK_SIZE) {
        for (size_t i = 0; i < CRYPTO_CEIO_BLOCK_SIZE; i++) {
            working[offset + i] ^= previous_block[i];
        }
        xtea_encrypt_block(working + offset, key_words);
        memcpy(previous_block, working + offset, CRYPTO_CEIO_BLOCK_SIZE);
    }

    secure_zero_memory(key_words, sizeof(key_words));
    *ciphertext = working;
    *ciphertext_size = working_size;
    return 0;
}

int crypto_decrypt_buffer(
    const unsigned char *ciphertext,
    size_t ciphertext_size,
    const unsigned char *key_material,
    size_t key_size,
    const unsigned char iv[CRYPTO_CEIO_IV_SIZE],
    unsigned char **plaintext,
    size_t *plaintext_size
) {
    uint32_t key_words[CRYPTO_CEIO_KEY_WORDS] = {0};
    unsigned char previous_block[CRYPTO_CEIO_BLOCK_SIZE];
    unsigned char current_block[CRYPTO_CEIO_BLOCK_SIZE];
    unsigned char *working = NULL;

    if (ciphertext == NULL || plaintext == NULL || plaintext_size == NULL) {
        return -1;
    }
    if (key_material == NULL || key_size == 0 || iv == NULL) {
        return -1;
    }
    if (ciphertext_size == 0 || (ciphertext_size % CRYPTO_CEIO_BLOCK_SIZE) != 0) {
        return -1;
    }
    if (derive_xtea_key(key_material, key_size, key_words) != 0) {
        return -1;
    }

    working = malloc(ciphertext_size);
    if (working == NULL) {
        secure_zero_memory(key_words, sizeof(key_words));
        return -1;
    }
    memcpy(working, ciphertext, ciphertext_size);

    memcpy(previous_block, iv, sizeof(previous_block));
    for (size_t offset = 0; offset < ciphertext_size; offset += CRYPTO_CEIO_BLOCK_SIZE) {
        memcpy(current_block, working + offset, CRYPTO_CEIO_BLOCK_SIZE);
        xtea_decrypt_block(working + offset, key_words);
        for (size_t i = 0; i < CRYPTO_CEIO_BLOCK_SIZE; i++) {
            working[offset + i] ^= previous_block[i];
        }
        memcpy(previous_block, current_block, CRYPTO_CEIO_BLOCK_SIZE);
    }

    secure_zero_memory(key_words, sizeof(key_words));

    if (remove_padding_and_validate_crc(working, ciphertext_size, plaintext, plaintext_size) != 0) {
        secure_zero_memory(working, ciphertext_size);
        free(working);
        return -1;
    }

    secure_zero_memory(working, ciphertext_size);
    free(working);
    return 0;
}
