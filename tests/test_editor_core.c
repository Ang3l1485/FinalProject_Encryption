#include "editor_core.h"

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

static void insert_text(EditorCore *core, const char *text) {
    for (size_t i = 0; text[i] != '\0'; i++) {
        CHECK(editor_core_insert_char(core, (unsigned char)text[i]) == EDITOR_CORE_OK);
    }
}

static void expect_buffer(EditorCore *core, const char *expected) {
    unsigned char *data = NULL;
    size_t size = 0;

    CHECK(editor_core_to_buffer(core, &data, &size) == EDITOR_CORE_OK);
    CHECK(size == strlen(expected));
    CHECK(memcmp(data, expected, size) == 0);

    free(data);
}

static void test_init(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_get_length(&core) == 0);
    CHECK(editor_core_get_cursor(&core) == 0);
    CHECK(editor_core_is_dirty(&core) == 0);

    editor_core_free(&core);
}

static void test_insert_text(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    insert_text(&core, "hello");

    CHECK(editor_core_get_length(&core) == 5);
    CHECK(editor_core_get_cursor(&core) == 5);
    CHECK(editor_core_is_dirty(&core) == 1);
    expect_buffer(&core, "hello");

    editor_core_free(&core);
}

static void test_backspace(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    insert_text(&core, "hello");
    CHECK(editor_core_backspace(&core) == EDITOR_CORE_OK);

    CHECK(editor_core_get_length(&core) == 4);
    CHECK(editor_core_get_cursor(&core) == 4);
    expect_buffer(&core, "hell");

    editor_core_free(&core);
}

static void test_move_and_insert_middle(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    insert_text(&core, "helo");
    CHECK(editor_core_move_to(&core, 2) == EDITOR_CORE_OK);
    CHECK(editor_core_insert_char(&core, 'l') == EDITOR_CORE_OK);

    CHECK(editor_core_get_cursor(&core) == 3);
    expect_buffer(&core, "hello");

    editor_core_free(&core);
}

static void test_delete_at_cursor(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    insert_text(&core, "abcXdef");
    CHECK(editor_core_move_to(&core, 3) == EDITOR_CORE_OK);
    CHECK(editor_core_delete(&core) == EDITOR_CORE_OK);

    expect_buffer(&core, "abcdef");
    CHECK(editor_core_get_cursor(&core) == 3);

    editor_core_free(&core);
}

static void test_move_left_and_right(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    insert_text(&core, "abc");

    CHECK(editor_core_move_left(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_move_left(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_get_cursor(&core) == 1);
    CHECK(editor_core_move_right(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_get_cursor(&core) == 2);
    expect_buffer(&core, "abc");

    editor_core_free(&core);
}

static void test_load_buffer(void) {
    EditorCore core;
    const unsigned char input[] = {'a', 'b', '\0', 'c'};
    unsigned char *data = NULL;
    size_t size = 0;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_load_buffer(&core, input, sizeof(input)) == EDITOR_CORE_OK);
    CHECK(editor_core_is_dirty(&core) == 0);
    CHECK(editor_core_get_length(&core) == sizeof(input));
    CHECK(editor_core_get_cursor(&core) == sizeof(input));

    CHECK(editor_core_to_buffer(&core, &data, &size) == EDITOR_CORE_OK);
    CHECK(size == sizeof(input));
    CHECK(memcmp(data, input, size) == 0);

    free(data);
    editor_core_free(&core);
}

static void test_dirty_flag(void) {
    EditorCore core;

    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_is_dirty(&core) == 0);

    CHECK(editor_core_insert_char(&core, 'x') == EDITOR_CORE_OK);
    CHECK(editor_core_is_dirty(&core) == 1);

    editor_core_mark_clean(&core);
    CHECK(editor_core_is_dirty(&core) == 0);

    CHECK(editor_core_backspace(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_is_dirty(&core) == 1);

    editor_core_free(&core);
}

static void test_invalid_arguments(void) {
    EditorCore core;
    unsigned char *data = NULL;
    size_t size = 0;

    CHECK(editor_core_init(NULL) == EDITOR_CORE_ERR_INVALID);
    CHECK(editor_core_insert_char(NULL, 'x') == EDITOR_CORE_ERR_INVALID);
    CHECK(editor_core_init(&core) == EDITOR_CORE_OK);
    CHECK(editor_core_to_buffer(NULL, &data, &size) == EDITOR_CORE_ERR_INVALID);
    CHECK(editor_core_load_buffer(&core, NULL, 1) == EDITOR_CORE_ERR_INVALID);
    CHECK(editor_core_move_to(&core, 1) == EDITOR_CORE_ERR_INVALID);

    editor_core_free(&core);
}

int main(void) {
    test_init();
    test_insert_text();
    test_backspace();
    test_move_and_insert_middle();
    test_delete_at_cursor();
    test_move_left_and_right();
    test_load_buffer();
    test_dirty_flag();
    test_invalid_arguments();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    puts("All editor core tests passed");
    return 0;
}
