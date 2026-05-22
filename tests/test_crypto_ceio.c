#include "crypto_ceio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
        return; \
    } \
} while (0)

static void test_roundtrip(void) {
    static const unsigned char plaintext[] =
        "La compresion debe ocurrir antes de la encriptacion.\n";
    static const unsigned char key_material[] = "clave-del-proyecto";
    unsigned char iv[CRYPTO_CEIO_IV_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7};
    unsigned char *ciphertext = NULL;
    size_t ciphertext_size = 0;
    unsigned char *decrypted = NULL;
    size_t decrypted_size = 0;

    CHECK(crypto_encrypt_buffer(
        plaintext,
        sizeof(plaintext) - 1,
        key_material,
        sizeof(key_material) - 1,
        iv,
        &ciphertext,
        &ciphertext_size
    ) == 0);
    CHECK(ciphertext_size > sizeof(plaintext) - 1);
    CHECK(memcmp(ciphertext, plaintext, sizeof(plaintext) - 1) != 0);

    CHECK(crypto_decrypt_buffer(
        ciphertext,
        ciphertext_size,
        key_material,
        sizeof(key_material) - 1,
        iv,
        &decrypted,
        &decrypted_size
    ) == 0);
    CHECK(decrypted_size == sizeof(plaintext) - 1);
    CHECK(memcmp(decrypted, plaintext, decrypted_size) == 0);

    free(ciphertext);
    free(decrypted);
}

static void test_wrong_key_fails(void) {
    static const unsigned char plaintext[] = "buffer de prueba";
    static const unsigned char correct_key[] = "clave-correcta";
    static const unsigned char wrong_key[] = "clave-incorrecta";
    unsigned char iv[CRYPTO_CEIO_IV_SIZE] = {8, 7, 6, 5, 4, 3, 2, 1};
    unsigned char *ciphertext = NULL;
    size_t ciphertext_size = 0;
    unsigned char *decrypted = NULL;
    size_t decrypted_size = 0;

    CHECK(crypto_encrypt_buffer(
        plaintext,
        sizeof(plaintext) - 1,
        correct_key,
        sizeof(correct_key) - 1,
        iv,
        &ciphertext,
        &ciphertext_size
    ) == 0);

    CHECK(crypto_decrypt_buffer(
        ciphertext,
        ciphertext_size,
        wrong_key,
        sizeof(wrong_key) - 1,
        iv,
        &decrypted,
        &decrypted_size
    ) != 0);

    free(ciphertext);
}

static void test_secure_zero_memory(void) {
    unsigned char secret[] = {'s', 'e', 'c', 'r', 'e', 't'};

    secure_zero_memory(secret, sizeof(secret));
    for (size_t i = 0; i < sizeof(secret); i++) {
        CHECK(secret[i] == 0);
    }
}

static void test_secure_key_copy(void) {
    static const unsigned char key_material[] = "clave-temporal";
    unsigned char *locked_copy = NULL;

    CHECK(crypto_secure_alloc_key_copy(
        key_material,
        sizeof(key_material) - 1,
        &locked_copy
    ) == 0);
    CHECK(locked_copy != NULL);
    CHECK(memcmp(locked_copy, key_material, sizeof(key_material) - 1) == 0);

    crypto_secure_free_key(locked_copy, sizeof(key_material) - 1);
}

static void test_generate_iv_or_fallback(void) {
    unsigned char iv[CRYPTO_CEIO_IV_SIZE] = {0};
    int result = crypto_generate_iv(iv);

    if (result == 0) {
        int any_non_zero = 0;
        for (size_t i = 0; i < sizeof(iv); i++) {
            if (iv[i] != 0) {
                any_non_zero = 1;
                break;
            }
        }
        CHECK(any_non_zero == 1);
    }
}

int main(void) {
    test_roundtrip();
    test_wrong_key_fails();
    test_secure_zero_memory();
    test_secure_key_copy();
    test_generate_iv_or_fallback();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    puts("All crypto_ceio tests passed");
    return 0;
}
