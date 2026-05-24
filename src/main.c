#include "editor_app.h"
#include "editor_ui_ncurses.h"
#include "io_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
    fprintf(
        stderr,
        "Usage: %s [--io=write|mmap] <archivo.ceio>\n",
        program_name
    );
}

static int parse_io_mode(const char *value, CeioIoMode *out_mode) {
    if (strcmp(value, "write") == 0) {
        *out_mode = CEIO_IO_WRITE;
        return 0;
    }
    if (strcmp(value, "mmap") == 0) {
        *out_mode = CEIO_IO_MMAP;
        return 0;
    }
    return -1;
}

int main(int argc, char **argv) {
    const char *filename = NULL;
    CeioIoMode io_mode = CEIO_IO_WRITE;
    EditorApp app;

    /*
     * main.c only coordinates startup and shutdown so the architecture stays
     * easy to explain: UI, editing logic, and compressed I/O live elsewhere.
     */
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--io=", 5) == 0) {
            if (parse_io_mode(argv[i] + 5, &io_mode) != 0) {
                fprintf(stderr, "Invalid I/O mode: %s\n", argv[i] + 5);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (filename == NULL) {
            filename = argv[i];
            continue;
        }
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (filename == NULL) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (editor_app_init(&app, filename, io_mode) != 0) {
        fprintf(stderr, "Could not initialize editor application\n");
        return EXIT_FAILURE;
    }

    int ui_result = editor_ui_ncurses_run(&app);
    if (ui_result != 0) {
        fprintf(stderr, "%s\n", editor_app_get_status(&app));
    }
    editor_app_free(&app);

    return ui_result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
