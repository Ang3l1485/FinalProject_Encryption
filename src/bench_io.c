#include "benchmark_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
    fprintf(
        stderr,
        "Usage: %s --mode=baseline|compressed-write|compressed-mmap|encrypted-write|encrypted-mmap --size-mb=50 --output=archivo\n",
        program_name
    );
}

int main(int argc, char **argv) {
    BenchmarkConfig config;
    config.mode = BENCHMARK_MODE_BASELINE;
    config.size_mb = 50;
    config.output_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--mode=", 7) == 0) {
            if (benchmark_parse_mode(argv[i] + 7, &config.mode) != 0) {
                fprintf(stderr, "Invalid benchmark mode\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            continue;
        }
        if (strncmp(argv[i], "--size-mb=", 10) == 0) {
            config.size_mb = (size_t)strtoul(argv[i] + 10, NULL, 10);
            continue;
        }
        if (strncmp(argv[i], "--output=", 9) == 0) {
            config.output_path = argv[i] + 9;
            continue;
        }
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    if (config.output_path == NULL || config.size_mb == 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /*
     * The benchmark entry point only parses arguments. Timing and syscall
     * metrics are collected externally with /usr/bin/time and strace -c.
     */
    return benchmark_run(&config) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
