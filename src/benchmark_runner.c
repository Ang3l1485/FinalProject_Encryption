#include "benchmark_runner.h"

#include "editor_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const unsigned char BENCH_KEY[] = "default_test_key";

static int write_all_small_chunks(int fd, const unsigned char *data, size_t size) {
    size_t offset = 0;
    const size_t chunk_size = 64u;

    while (offset < size) {
        size_t remaining = size - offset;
        size_t current = remaining < chunk_size ? remaining : chunk_size;
        ssize_t written = write(fd, data + offset, current);
        if (written <= 0) {
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int build_benchmark_text(size_t size_mb, unsigned char **out_data, size_t *out_size) {
    static const char pattern[] =
        "CEIO benchmark line: compressed terminal editors benefit from repeated text.\n";
    const size_t pattern_size = sizeof(pattern) - 1;
    const size_t target_size = size_mb * 1024u * 1024u;
    unsigned char *buffer = malloc(target_size);

    if (out_data == NULL || out_size == NULL || buffer == NULL) {
        free(buffer);
        return -1;
    }

    for (size_t offset = 0; offset < target_size; offset += pattern_size) {
        size_t copy_size = pattern_size;
        if (offset + copy_size > target_size) {
            copy_size = target_size - offset;
        }
        memcpy(buffer + offset, pattern, copy_size);
    }

    *out_data = buffer;
    *out_size = target_size;
    return 0;
}

static int get_file_size_bytes(const char *path, size_t *out_size) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    *out_size = (size_t)st.st_size;
    return 0;
}

static int run_baseline_plain(const BenchmarkConfig *config, const unsigned char *data, size_t size) {
    int fd = open(config->output_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }

    int result = write_all_small_chunks(fd, data, size);
    if (close(fd) != 0) {
        return -1;
    }
    return result;
}

static int run_compressed(const BenchmarkConfig *config, const unsigned char *data, size_t size) {
    CeioIoMode mode = config->mode == BENCHMARK_MODE_COMPRESSED_MMAP
        ? CEIO_IO_MMAP
        : CEIO_IO_WRITE;

    return editor_file_save(
        config->output_path,
        data,
        size,
        mode,
        BENCH_KEY,
        sizeof(BENCH_KEY) - 1
    );
}

int benchmark_parse_mode(const char *value, BenchmarkMode *out_mode) {
    if (value == NULL || out_mode == NULL) {
        return -1;
    }
    if (strcmp(value, "baseline") == 0) {
        *out_mode = BENCHMARK_MODE_BASELINE;
        return 0;
    }
    if (strcmp(value, "compressed-write") == 0) {
        *out_mode = BENCHMARK_MODE_COMPRESSED_WRITE;
        return 0;
    }
    if (strcmp(value, "compressed-mmap") == 0) {
        *out_mode = BENCHMARK_MODE_COMPRESSED_MMAP;
        return 0;
    }
    return -1;
}

const char *benchmark_mode_name(BenchmarkMode mode) {
    switch (mode) {
        case BENCHMARK_MODE_BASELINE:
            return "baseline-plain-small-writes";
        case BENCHMARK_MODE_COMPRESSED_WRITE:
            return "compressed-write";
        case BENCHMARK_MODE_COMPRESSED_MMAP:
            return "compressed-mmap";
        default:
            return "unknown";
    }
}

int benchmark_run(const BenchmarkConfig *config) {
    unsigned char *data = NULL;
    size_t original_size = 0;
    size_t final_size = 0;
    int result = -1;

    if (config == NULL || config->output_path == NULL || config->size_mb == 0) {
        return -1;
    }
    if (build_benchmark_text(config->size_mb, &data, &original_size) != 0) {
        return -1;
    }

    if (config->mode == BENCHMARK_MODE_BASELINE) {
        result = run_baseline_plain(config, data, original_size);
    } else {
        result = run_compressed(config, data, original_size);
    }
    if (result != 0) {
        free(data);
        return -1;
    }

    if (get_file_size_bytes(config->output_path, &final_size) != 0) {
        free(data);
        return -1;
    }

    double reduction = 0.0;
    if (original_size > 0 && final_size <= original_size) {
        reduction = 100.0 * (1.0 - ((double)final_size / (double)original_size));
    }

    printf("mode: %s\n", benchmark_mode_name(config->mode));
    printf("original_size_bytes: %zu\n", original_size);
    printf("final_size_bytes: %zu\n", final_size);
    printf("reduction_percent: %.2f\n", reduction);
    printf("output_file: %s\n", config->output_path);
    printf(
        "io_mode: %s\n",
        config->mode == BENCHMARK_MODE_COMPRESSED_MMAP ? "mmap" :
        (config->mode == BENCHMARK_MODE_COMPRESSED_WRITE ? "write" : "write-small-chunks")
    );

    free(data);
    return 0;
}