#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>

#define GAP_BUFFER_OK 0
#define GAP_BUFFER_ERR_INVALID -1
#define GAP_BUFFER_ERR_MEMORY -2

typedef struct {
    unsigned char *data;
    size_t capacity;
    size_t gap_start;
    size_t gap_end;
} GapBuffer;

int gap_buffer_init(GapBuffer *buffer, size_t initial_capacity);
void gap_buffer_free(GapBuffer *buffer);

int gap_buffer_insert_char(GapBuffer *buffer, unsigned char ch);
int gap_buffer_delete_before_cursor(GapBuffer *buffer);
int gap_buffer_delete_at_cursor(GapBuffer *buffer);

int gap_buffer_move_left(GapBuffer *buffer);
int gap_buffer_move_right(GapBuffer *buffer);
int gap_buffer_move_to(GapBuffer *buffer, size_t position);

size_t gap_buffer_length(const GapBuffer *buffer);
size_t gap_buffer_cursor(const GapBuffer *buffer);

int gap_buffer_to_buffer(const GapBuffer *buffer,
                         unsigned char **out_data,
                         size_t *out_size);
int gap_buffer_load_buffer(GapBuffer *buffer,
                           const unsigned char *data,
                           size_t size);

#endif
