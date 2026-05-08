#include "../include/editor_file.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    const char *text =
        "Benchmark persistence test";

    clock_t start = clock();

    for (int i = 0; i < 1000; i++) {

        save_ceio_file(
            "bench.ceio",
            (const unsigned char *)text,
            strlen(text)
        );

    }

    clock_t end = clock();

    double total =
        (double)(end - start)
        / CLOCKS_PER_SEC;

    printf(
        "Benchmark time: %f\n",
        total
    );

    return 0;
}