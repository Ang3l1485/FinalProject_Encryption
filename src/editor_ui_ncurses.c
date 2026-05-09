#include "editor_ui_ncurses.h"

#include "io_backend.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define CTRL_KEY(ch) ((ch) & 0x1f)

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

static void editor_ui_handle_key(EditorApp *app, int key_code) {
    switch (key_code) {
        case CTRL_KEY('s'):
            editor_app_save(app);
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

    while (editor_app_is_running(app)) {
        editor_ui_draw(app, &viewport);
        int key_code = getch();
        editor_ui_handle_key(app, key_code);
    }

    endwin();
    return 0;
}
