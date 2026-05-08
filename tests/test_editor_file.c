#include "../include/editor_file.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    const char *text =
        "Hola desde CEIO editor";

    if (save_ceio_file(
        "test.ceio",
        (const unsigned char *)text,
        strlen(text)
    ) != 0) {
        printf("Error saving file\n");
        return 1;
    }

    unsigned char *loaded = NULL;
    size_t size = 0;

    if (load_ceio_file(
        "test.ceio",
        &loaded,
        &size
    ) != 0) {
        printf("Error loading file\n");
        return 1;
    }

    printf(
        "Loaded text: %s\n",
        loaded
    );

    free(loaded);

    return 0;
}