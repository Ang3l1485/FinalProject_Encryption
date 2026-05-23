CC ?= gcc
CPPFLAGS ?= -D_DEFAULT_SOURCE -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS ?=

BUILD_DIR := build
RESULTS_DIR := results

LIBS_COMMON := -lz
LIBS_EDITOR := $(LIBS_COMMON) -lncurses

CORE_OBJECTS := \
	$(BUILD_DIR)/gap_buffer.o \
	$(BUILD_DIR)/editor_core.o

PERSISTENCE_OBJECTS := \
	$(BUILD_DIR)/compress_zlib.o \
	$(BUILD_DIR)/ceio_format.o \
	$(BUILD_DIR)/io_backend.o \
	$(BUILD_DIR)/editor_file.o

CRYPTO_OBJECTS := \
	$(BUILD_DIR)/crypto_ceio.o

APP_OBJECTS := \
	$(BUILD_DIR)/editor_app.o \
	$(BUILD_DIR)/editor_ui_ncurses.o

BENCH_OBJECTS := \
	$(BUILD_DIR)/benchmark_runner.o

.PHONY: all test valgrind clean profile

all: $(BUILD_DIR)/editor $(BUILD_DIR)/bench_io $(BUILD_DIR)/test_editor_core $(BUILD_DIR)/test_editor_file $(BUILD_DIR)/test_crypto_ceio

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESULTS_DIR):
	mkdir -p $(RESULTS_DIR)

$(BUILD_DIR)/gap_buffer.o: src/gap_buffer.c include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_core.o: src/editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/compress_zlib.o: src/compress_zlib.c include/compress_zlib.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ceio_format.o: src/ceio_format.c include/ceio_format.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/io_backend.o: src/io_backend.c include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_file.o: src/editor_file.c include/editor_file.h include/ceio_format.h include/compress_zlib.h include/io_backend.h include/crypto_ceio.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/crypto_ceio.o: src/crypto_ceio.c include/crypto_ceio.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_app.o: src/editor_app.c include/editor_app.h include/editor_core.h include/editor_file.h include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_ui_ncurses.o: src/editor_ui_ncurses.c include/editor_ui_ncurses.h include/editor_app.h include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/benchmark_runner.o: src/benchmark_runner.c include/benchmark_runner.h include/editor_file.h include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c include/editor_app.h include/editor_ui_ncurses.h include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bench_io.o: src/bench_io.c include/benchmark_runner.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_editor_core.o: tests/test_editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_editor_file.o: tests/test_editor_file.c include/editor_file.h include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_crypto_ceio.o: tests/test_crypto_ceio.c include/crypto_ceio.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor: $(BUILD_DIR)/main.o $(CORE_OBJECTS) $(PERSISTENCE_OBJECTS) $(CRYPTO_OBJECTS) $(APP_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS_EDITOR)

$(BUILD_DIR)/bench_io: $(BUILD_DIR)/bench_io.o $(PERSISTENCE_OBJECTS) $(CRYPTO_OBJECTS) $(BENCH_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS_COMMON)

$(BUILD_DIR)/test_editor_core: $(BUILD_DIR)/test_editor_core.o $(CORE_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_editor_file: $(BUILD_DIR)/test_editor_file.o $(PERSISTENCE_OBJECTS) $(CRYPTO_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS_COMMON)

$(BUILD_DIR)/test_crypto_ceio: $(BUILD_DIR)/test_crypto_ceio.o $(CRYPTO_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS_COMMON)

test: $(BUILD_DIR)/test_editor_core $(BUILD_DIR)/test_editor_file $(BUILD_DIR)/test_crypto_ceio
	./$(BUILD_DIR)/test_editor_core
	./$(BUILD_DIR)/test_editor_file
	./$(BUILD_DIR)/test_crypto_ceio

valgrind: $(BUILD_DIR)/test_editor_core $(BUILD_DIR)/test_editor_file $(BUILD_DIR)/test_crypto_ceio
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/test_editor_core
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/test_editor_file
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/test_crypto_ceio

profile: $(BUILD_DIR)/bench_io | $(RESULTS_DIR)
	chmod +x scripts/run_profile.sh
	./scripts/run_profile.sh

clean:
	rm -rf $(BUILD_DIR) $(RESULTS_DIR)