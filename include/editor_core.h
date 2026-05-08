#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

#include "gap_buffer.h"

#include <stddef.h>

#define EDITOR_CORE_OK 0
#define EDITOR_CORE_ERR_INVALID -1
#define EDITOR_CORE_ERR_MEMORY -2

typedef struct {
    GapBuffer buffer;
    int dirty;
    int initialized;
} EditorCore;

int editor_core_init(EditorCore *core);
void editor_core_free(EditorCore *core);

int editor_core_insert_char(EditorCore *core, unsigned char ch);
int editor_core_backspace(EditorCore *core);
int editor_core_delete(EditorCore *core);

int editor_core_move_left(EditorCore *core);
int editor_core_move_right(EditorCore *core);
int editor_core_move_to(EditorCore *core, size_t position);

size_t editor_core_get_cursor(const EditorCore *core);
size_t editor_core_get_length(const EditorCore *core);

int editor_core_to_buffer(const EditorCore *core,
                          unsigned char **out_data,
                          size_t *out_size);
int editor_core_load_buffer(EditorCore *core,
                            const unsigned char *data,
                            size_t size);

int editor_core_is_dirty(const EditorCore *core);
void editor_core_mark_clean(EditorCore *core);

#endif
