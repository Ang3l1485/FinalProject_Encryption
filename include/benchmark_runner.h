#ifndef BENCHMARK_RUNNER_H
#define BENCHMARK_RUNNER_H

#include "io_backend.h"

#include <stddef.h>

typedef enum {
    BENCHMARK_MODE_BASELINE = 0,
    BENCHMARK_MODE_COMPRESSED_WRITE = 1,
    BENCHMARK_MODE_COMPRESSED_MMAP = 2
} BenchmarkMode;

typedef struct {
    BenchmarkMode mode;
    size_t size_mb;
    const char *output_path;
} BenchmarkConfig;

int benchmark_parse_mode(const char *value, BenchmarkMode *out_mode);
const char *benchmark_mode_name(BenchmarkMode mode);
int benchmark_run(const BenchmarkConfig *config);

#endif
