#include "editor_core.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Temporary demo for stage 1.
 * This is not the final ncurses UI; it only exercises the in-memory editor core.
 */
int main(void) {
    EditorCore editor;
    unsigned char *text = NULL;
    size_t text_size = 0;

    if (editor_core_init(&editor) != EDITOR_CORE_OK) {
        puts("Could not initialize editor core");
        return 1;
    }

    const char *sample = "Hello C editor";
    for (size_t i = 0; sample[i] != '\0'; i++) {
        if (editor_core_insert_char(&editor, (unsigned char)sample[i]) != EDITOR_CORE_OK) {
            puts("Insert failed");
            editor_core_free(&editor);
            return 1;
        }
    }

    editor_core_move_to(&editor, 5);
    editor_core_insert_char(&editor, ',');
    editor_core_move_to(&editor, editor_core_get_length(&editor));
    editor_core_backspace(&editor);

    if (editor_core_to_buffer(&editor, &text, &text_size) != EDITOR_CORE_OK) {
        puts("Export failed");
        editor_core_free(&editor);
        return 1;
    }

    printf("Stage 1 demo output: %.*s\n", (int)text_size, text);
    printf("Length: %zu\n", editor_core_get_length(&editor));
    printf("Cursor: %zu\n", editor_core_get_cursor(&editor));
    printf("Dirty: %d\n", editor_core_is_dirty(&editor));

    free(text);
    editor_core_free(&editor);
    return 0;
}
