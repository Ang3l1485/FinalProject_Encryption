#include "editor_ui_ncurses.h"

#include "crypto_ceio.h"
#include "io_backend.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CTRL_KEY(ch) ((ch) & 0x1f)
#define EDITOR_UI_KEY_MAX 128u

typedef struct {
    int row_offset;
    int col_offset;
} EditorViewport;

static void draw_truncated_line(int row, int width, const char *text) {
    mvhline(row, 0, ' ', width);
    mvaddnstr(row, 0, text, width > 0 ? width : 0);
}

static void editor_ui_adjust_viewport(
    EditorViewport *viewport,
    int cursor_row,
    int cursor_col,
    int text_rows,
    int text_cols
) {
    if (cursor_row < viewport->row_offset) {
        viewport->row_offset = cursor_row;
    } else if (cursor_row >= viewport->row_offset + text_rows) {
        viewport->row_offset = cursor_row - text_rows + 1;
    }

    if (cursor_col < viewport->col_offset) {
        viewport->col_offset = cursor_col;
    } else if (cursor_col >= viewport->col_offset + text_cols) {
        viewport->col_offset = cursor_col - text_cols + 1;
    }

    if (viewport->row_offset < 0) {
        viewport->row_offset = 0;
    }
    if (viewport->col_offset < 0) {
        viewport->col_offset = 0;
    }
}

static void editor_ui_draw(EditorApp *app, EditorViewport *viewport) {
    unsigned char *data = NULL;
    size_t size = 0;
    size_t cursor = 0;
    int rows = 0;
    int cols = 0;
    int text_rows = 0;
    int text_cols = 0;
    int cursor_row = 0;
    int cursor_col = 0;

    getmaxyx(stdscr, rows, cols);
    text_rows = rows - 3;
    text_cols = cols;
    if (text_rows < 1) {
        text_rows = 1;
    }

    if (editor_app_snapshot(app, &data, &size, &cursor) != 0) {
        erase();
        draw_truncated_line(0, cols, "CEIO Editor | snapshot error");
        draw_truncated_line(1, cols, "Could not render buffer");
        refresh();
        return;
    }

    for (size_t i = 0; i < cursor && i < size; i++) {
        if (data[i] == '\n') {
            cursor_row++;
            cursor_col = 0;
        } else if (data[i] == '\t') {
            cursor_col += 4;
        } else {
            cursor_col++;
        }
    }

    editor_ui_adjust_viewport(viewport, cursor_row, cursor_col, text_rows, text_cols);

    erase();

    char header[512];
    snprintf(
        header,
        sizeof(header),
        "CEIO Editor | file: %s | mode: %s",
        editor_app_get_filename(app),
        ceio_io_mode_name(editor_app_get_io_mode(app))
    );
    attron(A_REVERSE);
    draw_truncated_line(0, cols, header);
    attroff(A_REVERSE);

    int row = 0;
    int col = 0;
    for (size_t i = 0; i < size; i++) {
        unsigned char ch = data[i];

        if (ch == '\n') {
            row++;
            col = 0;
            continue;
        }

        int screen_row = row - viewport->row_offset + 1;
        int width = (ch == '\t') ? 4 : 1;
        if (row >= viewport->row_offset &&
            col + width > viewport->col_offset &&
            screen_row >= 1 &&
            screen_row < rows - 2) {
            int visible_col = col - viewport->col_offset;
            if (visible_col < 0) {
                visible_col = 0;
            }
            if (visible_col < cols) {
                if (ch == '\t') {
                    for (int j = 0; j < width && visible_col + j < cols; j++) {
                        mvaddch(screen_row, visible_col + j, ' ');
                    }
                } else if (isprint(ch)) {
                    mvaddch(screen_row, visible_col, ch);
                } else {
                    mvaddch(screen_row, visible_col, '.');
                }
            }
        }

        col += width;
    }

    attron(A_REVERSE);
    draw_truncated_line(rows - 2, cols, "Ctrl+S Save | Ctrl+Q/F10/Esc Quit | Arrows Move | Backspace Delete");
    attroff(A_REVERSE);
    draw_truncated_line(rows - 1, cols, editor_app_get_status(app));

    int final_cursor_row = cursor_row - viewport->row_offset + 1;
    int final_cursor_col = cursor_col - viewport->col_offset;

    if (final_cursor_row >= 1 && final_cursor_row < rows - 2 &&
        final_cursor_col >= 0 && final_cursor_col < cols) {
        move(final_cursor_row, final_cursor_col);
    } else {
        move(rows - 1, 0);
    }

    refresh();
    free(data);
}

static int editor_ui_prompt_hidden(
    const char *prompt,
    unsigned char *buffer,
    size_t capacity,
    size_t *out_size
) {
    size_t length = 0;
    int rows = 0;
    int cols = 0;

    if (prompt == NULL || buffer == NULL || capacity == 0 || out_size == NULL) {
        return -1;
    }

    memset(buffer, 0, capacity);
    getmaxyx(stdscr, rows, cols);
    curs_set(1);

    for (;;) {
        attron(A_REVERSE);
        draw_truncated_line(rows - 1, cols, prompt);
        attroff(A_REVERSE);
        move(rows - 1, (int)strnlen(prompt, (size_t)cols));
        refresh();

        int key_code = getch();
        if (key_code == '\n' || key_code == '\r') {
            if (length == 0) {
                beep();
                continue;
            }
            *out_size = length;
            return 0;
        }

        if (key_code == 27 || key_code == CTRL_KEY('q')) {
            secure_zero_memory(buffer, capacity);
            return -1;
        }

        if (key_code == KEY_BACKSPACE || key_code == 127 || key_code == '\b') {
            if (length > 0) {
                length--;
                buffer[length] = 0;
            }
            continue;
        }

        if (key_code >= 32 && key_code <= 126) {
            if (length + 1u >= capacity) {
                beep();
                continue;
            }
            buffer[length] = (unsigned char)key_code;
            length++;
        }
    }
}

static int editor_ui_prompt_and_store_key(EditorApp *app, const char *prompt) {
    unsigned char key_buffer[EDITOR_UI_KEY_MAX];
    size_t key_size = 0;
    int result = -1;

    if (editor_ui_prompt_hidden(prompt, key_buffer, sizeof(key_buffer), &key_size) != 0) {
        editor_app_set_status(app, "Key entry cancelled");
        return -1;
    }

    result = editor_app_set_key(app, key_buffer, key_size);
    secure_zero_memory(key_buffer, sizeof(key_buffer));

    if (result != 0) {
        editor_app_set_status(app, "Could not store key");
        return -1;
    }

    return 0;
}

static int editor_ui_ensure_key(EditorApp *app, const char *prompt) {
    if (editor_app_has_key(app)) {
        return 0;
    }
    return editor_ui_prompt_and_store_key(app, prompt);
}

static int editor_ui_load_existing_file(EditorApp *app, EditorViewport *viewport) {
    const char *filename = editor_app_get_filename(app);

    if (access(filename, F_OK) != 0) {
        return 0;
    }

    for (int attempt = 0; attempt < 3; attempt++) {
        editor_ui_draw(app, viewport);
        if (editor_ui_prompt_and_store_key(app, "Key to open file: ") != 0) {
            return -1;
        }
        if (editor_app_load(app) == 0) {
            editor_app_clear_key(app);
            return 0;
        }
        editor_app_clear_key(app);
        editor_app_set_status(app, "Load failed; check key");
    }

    return -1;
}

static void editor_ui_save(EditorApp *app) {
    if (editor_ui_ensure_key(app, "Key to save file: ") != 0) {
        return;
    }
    editor_app_save(app);
    editor_app_clear_key(app);
}

static void editor_ui_handle_key(EditorApp *app, int key_code) {
    switch (key_code) {
        case CTRL_KEY('s'):
            editor_ui_save(app);
            break;
        case CTRL_KEY('q'):
        case KEY_F(10):
            editor_app_request_quit(app);
            break;
        case KEY_LEFT:
            editor_app_move_left(app);
            break;
        case KEY_RIGHT:
            editor_app_move_right(app);
            break;
        case KEY_UP:
            editor_app_move_up(app);
            break;
        case KEY_DOWN:
            editor_app_move_down(app);
            break;
        case KEY_BACKSPACE:
        case 127:
        case '\b':
            editor_app_backspace(app);
            break;
        case KEY_DC:
            editor_app_delete(app);
            break;
        case '\n':
        case '\r':
            editor_app_insert_char(app, '\n');
            break;
        case '\t':
            editor_app_insert_char(app, '\t');
            break;
        case 27:
            editor_app_request_quit(app);
            break;
        default:
            if (key_code >= 32 && key_code <= 126) {
                editor_app_insert_char(app, (unsigned char)key_code);
            }
            break;
    }
}

int editor_ui_ncurses_run(EditorApp *app) {
    EditorViewport viewport = {0, 0};

    if (app == NULL) {
        return -1;
    }

    /*
     * ncurses is only responsible for terminal input/output. Compression,
     * file format handling, and storage backends remain outside the UI layer.
     */
    initscr();
    /*
     * raw() disables terminal flow-control shortcuts such as Ctrl+S/Ctrl+Q so
     * ncurses can deliver them to the editor and the application can decide
     * whether to save or quit.
     */
    raw();
    noecho();
    keypad(stdscr, TRUE);

    if (editor_ui_load_existing_file(app, &viewport) != 0) {
        endwin();
        return -1;
    }

    while (editor_app_is_running(app)) {
        editor_ui_draw(app, &viewport);
        int key_code = getch();
        editor_ui_handle_key(app, key_code);
    }

    endwin();
    return 0;
}
