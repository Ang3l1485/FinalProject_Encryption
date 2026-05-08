#include "gap_buffer.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_GAP_SIZE 64u

static size_t gap_size(const GapBuffer *buffer) {
    return buffer->gap_end - buffer->gap_start;
}

static int is_valid_buffer(const GapBuffer *buffer) {
    return buffer != NULL &&
           buffer->data != NULL &&
           buffer->gap_start <= buffer->gap_end &&
           buffer->gap_end <= buffer->capacity;
}

static int ensure_gap(GapBuffer *buffer, size_t needed) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (gap_size(buffer) >= needed) {
        return GAP_BUFFER_OK;
    }

    size_t text_size = gap_buffer_length(buffer);
    size_t required = text_size + needed + DEFAULT_GAP_SIZE;
    if (required < text_size || required < needed) {
        return GAP_BUFFER_ERR_MEMORY;
    }

    size_t new_capacity = buffer->capacity;
    while (new_capacity < required) {
        size_t next = new_capacity * 2u;
        if (next <= new_capacity) {
            new_capacity = required;
            break;
        }
        new_capacity = next;
    }

    unsigned char *new_data = realloc(buffer->data, new_capacity);
    if (new_data == NULL) {
        return GAP_BUFFER_ERR_MEMORY;
    }

    /* Move the suffix to the end so the enlarged gap stays at the cursor. */
    size_t suffix_size = buffer->capacity - buffer->gap_end;
    size_t new_gap_end = new_capacity - suffix_size;
    memmove(new_data + new_gap_end, new_data + buffer->gap_end, suffix_size);

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    buffer->gap_end = new_gap_end;
    return GAP_BUFFER_OK;
}

int gap_buffer_init(GapBuffer *buffer, size_t initial_capacity) {
    if (buffer == NULL) {
        return GAP_BUFFER_ERR_INVALID;
    }

    if (initial_capacity < DEFAULT_GAP_SIZE) {
        initial_capacity = DEFAULT_GAP_SIZE;
    }

    buffer->data = calloc(initial_capacity, sizeof(unsigned char));
    if (buffer->data == NULL) {
        buffer->capacity = 0;
        buffer->gap_start = 0;
        buffer->gap_end = 0;
        return GAP_BUFFER_ERR_MEMORY;
    }

    buffer->capacity = initial_capacity;
    buffer->gap_start = 0;
    buffer->gap_end = initial_capacity;
    return GAP_BUFFER_OK;
}

void gap_buffer_free(GapBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }

    free(buffer->data);
    buffer->data = NULL;
    buffer->capacity = 0;
    buffer->gap_start = 0;
    buffer->gap_end = 0;
}

int gap_buffer_insert_char(GapBuffer *buffer, unsigned char ch) {
    int result = ensure_gap(buffer, 1u);
    if (result != GAP_BUFFER_OK) {
        return result;
    }

    buffer->data[buffer->gap_start] = ch;
    buffer->gap_start++;
    return GAP_BUFFER_OK;
}

int gap_buffer_delete_before_cursor(GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (buffer->gap_start > 0) {
        buffer->gap_start--;
    }
    return GAP_BUFFER_OK;
}

int gap_buffer_delete_at_cursor(GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (buffer->gap_end < buffer->capacity) {
        buffer->gap_end++;
    }
    return GAP_BUFFER_OK;
}

int gap_buffer_move_left(GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (buffer->gap_start == 0) {
        return GAP_BUFFER_OK;
    }

    buffer->gap_start--;
    buffer->gap_end--;
    buffer->data[buffer->gap_end] = buffer->data[buffer->gap_start];
    return GAP_BUFFER_OK;
}

int gap_buffer_move_right(GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (buffer->gap_end == buffer->capacity) {
        return GAP_BUFFER_OK;
    }

    buffer->data[buffer->gap_start] = buffer->data[buffer->gap_end];
    buffer->gap_start++;
    buffer->gap_end++;
    return GAP_BUFFER_OK;
}

int gap_buffer_move_to(GapBuffer *buffer, size_t position) {
    if (!is_valid_buffer(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }
    if (position > gap_buffer_length(buffer)) {
        return GAP_BUFFER_ERR_INVALID;
    }

    while (buffer->gap_start > position) {
        int result = gap_buffer_move_left(buffer);
        if (result != GAP_BUFFER_OK) {
            return result;
        }
    }
    while (buffer->gap_start < position) {
        int result = gap_buffer_move_right(buffer);
        if (result != GAP_BUFFER_OK) {
            return result;
        }
    }

    return GAP_BUFFER_OK;
}

size_t gap_buffer_length(const GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return 0;
    }
    return buffer->capacity - gap_size(buffer);
}

size_t gap_buffer_cursor(const GapBuffer *buffer) {
    if (!is_valid_buffer(buffer)) {
        return 0;
    }
    return buffer->gap_start;
}

int gap_buffer_to_buffer(const GapBuffer *buffer,
                         unsigned char **out_data,
                         size_t *out_size) {
    if (!is_valid_buffer(buffer) || out_data == NULL || out_size == NULL) {
        return GAP_BUFFER_ERR_INVALID;
    }

    size_t prefix_size = buffer->gap_start;
    size_t suffix_size = buffer->capacity - buffer->gap_end;
    size_t text_size = prefix_size + suffix_size;

    unsigned char *flat = NULL;
    if (text_size > 0) {
        flat = malloc(text_size);
        if (flat == NULL) {
            return GAP_BUFFER_ERR_MEMORY;
        }
        memcpy(flat, buffer->data, prefix_size);
        memcpy(flat + prefix_size, buffer->data + buffer->gap_end, suffix_size);
    }

    *out_data = flat;
    *out_size = text_size;
    return GAP_BUFFER_OK;
}

int gap_buffer_load_buffer(GapBuffer *buffer,
                           const unsigned char *data,
                           size_t size) {
    if (buffer == NULL || (data == NULL && size > 0)) {
        return GAP_BUFFER_ERR_INVALID;
    }

    size_t new_capacity = size + DEFAULT_GAP_SIZE;
    if (new_capacity < size) {
        return GAP_BUFFER_ERR_MEMORY;
    }

    unsigned char *new_data = calloc(new_capacity, sizeof(unsigned char));
    if (new_data == NULL) {
        return GAP_BUFFER_ERR_MEMORY;
    }

    if (size > 0) {
        memcpy(new_data, data, size);
    }

    free(buffer->data);
    buffer->data = new_data;
    buffer->capacity = new_capacity;
    buffer->gap_start = size;
    buffer->gap_end = new_capacity;
    return GAP_BUFFER_OK;
}
