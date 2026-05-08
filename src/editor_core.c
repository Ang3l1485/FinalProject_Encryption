#include "editor_core.h"

static int is_valid_core(const EditorCore *core) {
    return core != NULL && core->initialized;
}

static int map_gap_result(int result) {
    if (result == GAP_BUFFER_OK) {
        return EDITOR_CORE_OK;
    }
    if (result == GAP_BUFFER_ERR_MEMORY) {
        return EDITOR_CORE_ERR_MEMORY;
    }
    return EDITOR_CORE_ERR_INVALID;
}

int editor_core_init(EditorCore *core) {
    if (core == NULL) {
        return EDITOR_CORE_ERR_INVALID;
    }

    int result = gap_buffer_init(&core->buffer, 0);
    if (result != GAP_BUFFER_OK) {
        core->dirty = 0;
        core->initialized = 0;
        return map_gap_result(result);
    }

    core->dirty = 0;
    core->initialized = 1;
    return EDITOR_CORE_OK;
}

void editor_core_free(EditorCore *core) {
    if (core == NULL) {
        return;
    }

    gap_buffer_free(&core->buffer);
    core->dirty = 0;
    core->initialized = 0;
}

int editor_core_insert_char(EditorCore *core, unsigned char ch) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }

    int result = gap_buffer_insert_char(&core->buffer, ch);
    if (result == GAP_BUFFER_OK) {
        core->dirty = 1;
    }
    return map_gap_result(result);
}

int editor_core_backspace(EditorCore *core) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }

    size_t old_length = gap_buffer_length(&core->buffer);
    int result = gap_buffer_delete_before_cursor(&core->buffer);
    if (result == GAP_BUFFER_OK && gap_buffer_length(&core->buffer) != old_length) {
        core->dirty = 1;
    }
    return map_gap_result(result);
}

int editor_core_delete(EditorCore *core) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }

    size_t old_length = gap_buffer_length(&core->buffer);
    int result = gap_buffer_delete_at_cursor(&core->buffer);
    if (result == GAP_BUFFER_OK && gap_buffer_length(&core->buffer) != old_length) {
        core->dirty = 1;
    }
    return map_gap_result(result);
}

int editor_core_move_left(EditorCore *core) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }
    return map_gap_result(gap_buffer_move_left(&core->buffer));
}

int editor_core_move_right(EditorCore *core) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }
    return map_gap_result(gap_buffer_move_right(&core->buffer));
}

int editor_core_move_to(EditorCore *core, size_t position) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }
    return map_gap_result(gap_buffer_move_to(&core->buffer, position));
}

size_t editor_core_get_cursor(const EditorCore *core) {
    if (!is_valid_core(core)) {
        return 0;
    }
    return gap_buffer_cursor(&core->buffer);
}

size_t editor_core_get_length(const EditorCore *core) {
    if (!is_valid_core(core)) {
        return 0;
    }
    return gap_buffer_length(&core->buffer);
}

int editor_core_to_buffer(const EditorCore *core,
                          unsigned char **out_data,
                          size_t *out_size) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }
    return map_gap_result(gap_buffer_to_buffer(&core->buffer, out_data, out_size));
}

int editor_core_load_buffer(EditorCore *core,
                            const unsigned char *data,
                            size_t size) {
    if (!is_valid_core(core)) {
        return EDITOR_CORE_ERR_INVALID;
    }

    int result = gap_buffer_load_buffer(&core->buffer, data, size);
    if (result == GAP_BUFFER_OK) {
        core->dirty = 0;
    }
    return map_gap_result(result);
}

int editor_core_is_dirty(const EditorCore *core) {
    if (!is_valid_core(core)) {
        return 0;
    }
    return core->dirty;
}

void editor_core_mark_clean(EditorCore *core) {
    if (!is_valid_core(core)) {
        return;
    }
    core->dirty = 0;
}
