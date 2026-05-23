#include "editor_file.h"
#include "io_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
        return; \
    } \
} while (0)

static const unsigned char TEST_KEY[] = "default_test_key";
static const unsigned char WRONG_KEY[] = "wrong_test_key___";

static int contains_plaintext(
    const unsigned char *haystack,
    size_t haystack_size,
    const unsigned char *needle,
    size_t needle_size
) {
    if (needle_size == 0 || haystack_size < needle_size) {
        return 0;
    }

    for (size_t i = 0; i + needle_size <= haystack_size; i++) {
        if (memcmp(haystack + i, needle, needle_size) == 0) {
            return 1;
        }
    }
    return 0;
}

static void expect_roundtrip(const char *path, CeioIoMode mode) {
    const unsigned char input[] =
        "Hola desde CEIO editor\nLa persistencia final debe quedar comprimida.\n";
    unsigned char *loaded = NULL;
    size_t size = 0;
    unsigned char *raw_file = NULL;
    size_t raw_size = 0;

    CHECK(editor_file_save(
        path,
        input,
        sizeof(input) - 1,
        mode,
        TEST_KEY,
        sizeof(TEST_KEY) - 1
    ) == 0);

    CHECK(editor_file_load(
        path,
        &loaded,
        &size,
        TEST_KEY,
        sizeof(TEST_KEY) - 1
    ) == 0);

    CHECK(size == sizeof(input) - 1);
    CHECK(memcmp(loaded, input, size) == 0);

    CHECK(io_backend_read_file(path, &raw_file, &raw_size) == 0);
    CHECK(raw_size > 0);
    CHECK(contains_plaintext(raw_file, raw_size, input, sizeof(input) - 1) == 0);

    free(raw_file);
    free(loaded);
    unlink(path);
}

static void expect_wrong_key_fails(const char *path, CeioIoMode mode) {
    const unsigned char input[] =
        "Hola desde CEIO editor\nLa persistencia final debe quedar comprimida.\n";
    unsigned char *loaded = NULL;
    size_t size = 0;

    CHECK(editor_file_save(
        path,
        input,
        sizeof(input) - 1,
        mode,
        TEST_KEY,
        sizeof(TEST_KEY) - 1
    ) == 0);

    CHECK(editor_file_load(
        path,
        &loaded,
        &size,
        WRONG_KEY,
        sizeof(WRONG_KEY) - 1
    ) != 0);

    free(loaded);
    unlink(path);
}

static void test_write_mode(void) {
    expect_roundtrip("test_write_mode.ceio", CEIO_IO_WRITE);
    expect_wrong_key_fails("test_write_mode_wrong_key.ceio", CEIO_IO_WRITE);
}

static void test_mmap_mode(void) {
    expect_roundtrip("test_mmap_mode.ceio", CEIO_IO_MMAP);
    expect_wrong_key_fails("test_mmap_mode_wrong_key.ceio", CEIO_IO_MMAP);
}

int main(void) {
    test_write_mode();
    test_mmap_mode();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    puts("All editor file tests passed");
    return 0;
}