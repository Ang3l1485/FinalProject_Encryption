#include "editor_app.h"

#include "editor_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *editor_app_strdup(const char *text) {
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static void editor_app_set_status(EditorApp *app, const char *message) {
    if (app == NULL || message == NULL) {
        return;
    }

    snprintf(app->status_message, sizeof(app->status_message), "%s", message);
}

static int editor_app_move_vertical(EditorApp *app, int direction) {
    unsigned char *data = NULL;
    size_t size = 0;
    size_t cursor = 0;
    size_t line_start = 0;
    size_t line_end = 0;
    size_t column = 0;
    size_t target_line_start = 0;
    size_t target_line_length = 0;
    size_t target_cursor = 0;

    if (editor_app_snapshot(app, &data, &size, &cursor) != 0) {
        editor_app_set_status(app, "Could not inspect buffer");
        return -1;
    }

    line_start = cursor;
    while (line_start > 0 && data[line_start - 1] != '\n') {
        line_start--;
    }

    line_end = cursor;
    while (line_end < size && data[line_end] != '\n') {
        line_end++;
    }

    column = cursor - line_start;

    if (direction < 0) {
        if (line_start == 0) {
            free(data);
            return 0;
        }
        size_t previous_line_end = line_start - 1;
        target_line_start = previous_line_end;
        while (target_line_start > 0 && data[target_line_start - 1] != '\n') {
            target_line_start--;
        }
        target_line_length = previous_line_end - target_line_start;
    } else {
        if (line_end >= size) {
            free(data);
            return 0;
        }
        target_line_start = line_end + 1;
        while (target_line_start + target_line_length < size &&
               data[target_line_start + target_line_length] != '\n') {
            target_line_length++;
        }
    }

    target_cursor = target_line_start + (column < target_line_length ? column : target_line_length);
    free(data);
    return editor_core_move_to(&app->core, target_cursor);
}

int editor_app_init(EditorApp *app, const char *filename, CeioIoMode mode) {
    if (app == NULL || filename == NULL) {
        return -1;
    }
    if (editor_core_init(&app->core) != 0) {
        return -1;
    }

    app->filename = editor_app_strdup(filename);
    if (app->filename == NULL) {
        editor_core_free(&app->core);
        return -1;
    }

    app->io_mode = mode;
    app->running = 1;
    app->last_error = 0;
    editor_app_set_status(app, "Ready");
    return 0;
}

void editor_app_free(EditorApp *app) {
    if (app == NULL) {
        return;
    }

    free(app->filename);
    app->filename = NULL;
    editor_core_free(&app->core);
}

int editor_app_load(EditorApp *app) {
    unsigned char *buffer = NULL;
    size_t size = 0;

    if (app == NULL) {
        return -1;
    }
    if (editor_file_load(app->filename, &buffer, &size) != 0) {
        app->last_error = -1;
        editor_app_set_status(app, "Load failed");
        return -1;
    }

    int result = editor_core_load_buffer(&app->core, buffer, size);
    /* buffer comes from editor_file_load and must be released after importing. */
    free(buffer);
    if (result != 0) {
        app->last_error = result;
        editor_app_set_status(app, "Could not import file into editor");
        return -1;
    }

    editor_core_mark_clean(&app->core);
    app->last_error = 0;
    editor_app_set_status(app, "File loaded");
    return 0;
}

int editor_app_save(EditorApp *app) {
    unsigned char *buffer = NULL;
    size_t size = 0;

    if (app == NULL) {
        return -1;
    }
    if (editor_core_to_buffer(&app->core, &buffer, &size) != 0) {
        app->last_error = -1;
        editor_app_set_status(app, "Could not export text");
        return -1;
    }

    /*
     * The real editor never writes plain text to disk. It exports a temporary
     * flat buffer only to hand it to the compressed persistence pipeline.
     */
    int result = editor_file_save(app->filename, buffer, size, app->io_mode);
    /* buffer comes from editor_core_to_buffer and is released after saving. */
    free(buffer);

    if (result != 0) {
        app->last_error = result;
        editor_app_set_status(app, "Save failed");
        return -1;
    }

    editor_core_mark_clean(&app->core);
    app->last_error = 0;
    editor_app_set_status(app, "File saved");
    return 0;
}

int editor_app_insert_char(EditorApp *app, unsigned char ch) {
    int result = editor_core_insert_char(&app->core, ch);
    if (result != 0) {
        app->last_error = result;
        editor_app_set_status(app, "Insert failed");
    }
    return result;
}

int editor_app_backspace(EditorApp *app) {
    int result = editor_core_backspace(&app->core);
    if (result != 0) {
        app->last_error = result;
        editor_app_set_status(app, "Backspace failed");
    }
    return result;
}

int editor_app_delete(EditorApp *app) {
    int result = editor_core_delete(&app->core);
    if (result != 0) {
        app->last_error = result;
        editor_app_set_status(app, "Delete failed");
    }
    return result;
}

int editor_app_move_left(EditorApp *app) {
    return editor_core_move_left(&app->core);
}

int editor_app_move_right(EditorApp *app) {
    return editor_core_move_right(&app->core);
}

int editor_app_move_up(EditorApp *app) {
    return editor_app_move_vertical(app, -1);
}

int editor_app_move_down(EditorApp *app) {
    return editor_app_move_vertical(app, 1);
}

int editor_app_snapshot(
    const EditorApp *app,
    unsigned char **out_data,
    size_t *out_size,
    size_t *out_cursor
) {
    if (app == NULL || out_data == NULL || out_size == NULL || out_cursor == NULL) {
        return -1;
    }

    *out_cursor = editor_core_get_cursor(&app->core);
    return editor_core_to_buffer(&app->core, out_data, out_size);
}

void editor_app_request_quit(EditorApp *app) {
    if (app != NULL) {
        app->running = 0;
    }
}

int editor_app_is_running(const EditorApp *app) {
    return app != NULL && app->running;
}

const char *editor_app_get_status(const EditorApp *app) {
    return app != NULL ? app->status_message : "";
}

const char *editor_app_get_filename(const EditorApp *app) {
    return app != NULL && app->filename != NULL ? app->filename : "";
}

CeioIoMode editor_app_get_io_mode(const EditorApp *app) {
    return app != NULL ? app->io_mode : CEIO_IO_WRITE;
}
