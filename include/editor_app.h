#ifndef EDITOR_APP_H
#define EDITOR_APP_H

#include "editor_core.h"
#include "io_backend.h"

#include <stddef.h>

typedef struct {
    EditorCore core;
    char *filename;
    CeioIoMode io_mode;
    unsigned char *key;
    size_t key_size;
    int running;
    char status_message[256];
    int last_error;
} EditorApp;

int editor_app_init(EditorApp *app, const char *filename, CeioIoMode mode);
void editor_app_free(EditorApp *app);

int editor_app_load(EditorApp *app);
int editor_app_save(EditorApp *app);

int editor_app_set_key(EditorApp *app, const unsigned char *key, size_t key_size);
void editor_app_clear_key(EditorApp *app);
int editor_app_has_key(const EditorApp *app);
void editor_app_set_status(EditorApp *app, const char *message);

int editor_app_insert_char(EditorApp *app, unsigned char ch);
int editor_app_backspace(EditorApp *app);
int editor_app_delete(EditorApp *app);
int editor_app_move_left(EditorApp *app);
int editor_app_move_right(EditorApp *app);
int editor_app_move_up(EditorApp *app);
int editor_app_move_down(EditorApp *app);

int editor_app_snapshot(
    const EditorApp *app,
    unsigned char **out_data,
    size_t *out_size,
    size_t *out_cursor
);

void editor_app_request_quit(EditorApp *app);
int editor_app_is_running(const EditorApp *app);
const char *editor_app_get_status(const EditorApp *app);
const char *editor_app_get_filename(const EditorApp *app);
CeioIoMode editor_app_get_io_mode(const EditorApp *app);

#endif
