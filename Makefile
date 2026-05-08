CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g

LIBS := -lz

BUILD_DIR := build

CORE_OBJECTS := \
	$(BUILD_DIR)/gap_buffer.o \
	$(BUILD_DIR)/editor_core.o

PERSISTENCE_OBJECTS := \
	$(BUILD_DIR)/compress_zlib.o \
	$(BUILD_DIR)/io_backend.o \
	$(BUILD_DIR)/editor_file.o

.PHONY: all test valgrind clean

all: \
	$(BUILD_DIR)/editor_core_demo \
	$(BUILD_DIR)/test_editor_core \
	$(BUILD_DIR)/test_editor_file \
	$(BUILD_DIR)/bench_io

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/gap_buffer.o: src/gap_buffer.c include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_core.o: src/editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/compress_zlib.o: src/compress_zlib.c include/compress_zlib.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/io_backend.o: src/io_backend.c include/io_backend.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_file.o: src/editor_file.c include/editor_file.h include/compress_zlib.h include/io_backend.h include/ceio_format.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_editor_core.o: tests/test_editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_editor_file.o: tests/test_editor_file.c include/editor_file.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_core_demo: $(BUILD_DIR)/main.o $(CORE_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_editor_core: $(BUILD_DIR)/test_editor_core.o $(CORE_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_editor_file: \
	$(BUILD_DIR)/test_editor_file.o \
	$(PERSISTENCE_OBJECTS)

	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/bench_io: \
	src/bench_io.c \
	$(PERSISTENCE_OBJECTS)

	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LIBS)

test: $(BUILD_DIR)/test_editor_core $(BUILD_DIR)/test_editor_file
	./$(BUILD_DIR)/test_editor_core
	./$(BUILD_DIR)/test_editor_file

valgrind: $(BUILD_DIR)/test_editor_core
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/test_editor_core

clean:
	rm -rf $(BUILD_DIR)