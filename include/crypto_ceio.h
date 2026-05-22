#ifndef CRYPTO_CEIO_H
#define CRYPTO_CEIO_H

#include <stddef.h>
#include <stdint.h>

#define CRYPTO_CEIO_BLOCK_SIZE 8u
#define CRYPTO_CEIO_IV_SIZE CRYPTO_CEIO_BLOCK_SIZE
#define CRYPTO_CEIO_KEY_WORDS 4u
#define CRYPTO_CEIO_KEY_SIZE 16u

/*
 * This module provides an in-memory symmetric encryption layer intended to be
 * inserted after compression and before the final write() or mmap() step.
 *
 * Algorithm details:
 * - Cipher: XTEA
 * - Mode: CBC
 * - IV: 8 bytes, generated per message
 * - Padding: PKCS#7
 * - Integrity check: crc32 of plaintext stored inside the encrypted payload
 *
 * The encrypted payload format returned by crypto_encrypt_buffer is:
 *   [4 bytes crc32 little-endian][plaintext][PKCS#7 padding]
 *
 * The caller must store the IV alongside the ciphertext in the .ceio header or
 * adjacent metadata. The IV is not secret, but it must be unique per message.
 */

int crypto_generate_iv(unsigned char iv[CRYPTO_CEIO_IV_SIZE]);

int crypto_encrypt_buffer(
    const unsigned char *plaintext,
    size_t plaintext_size,
    const unsigned char *key_material,
    size_t key_size,
    const unsigned char iv[CRYPTO_CEIO_IV_SIZE],
    unsigned char **ciphertext,
    size_t *ciphertext_size
);

int crypto_decrypt_buffer(
    const unsigned char *ciphertext,
    size_t ciphertext_size,
    const unsigned char *key_material,
    size_t key_size,
    const unsigned char iv[CRYPTO_CEIO_IV_SIZE],
    unsigned char **plaintext,
    size_t *plaintext_size
);

int crypto_secure_alloc_key_copy(
    const unsigned char *source,
    size_t size,
    unsigned char **locked_copy
);

void crypto_secure_free_key(
    unsigned char *key,
    size_t size
);

int crypto_try_lock_memory(
    void *ptr,
    size_t size
);

void crypto_try_unlock_memory(
    void *ptr,
    size_t size
);

void secure_zero_memory(
    void *ptr,
    size_t size
);

#endif
