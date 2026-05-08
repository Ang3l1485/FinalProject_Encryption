CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g

BUILD_DIR := build

CORE_OBJECTS := \
	$(BUILD_DIR)/gap_buffer.o \
	$(BUILD_DIR)/editor_core.o

.PHONY: all test valgrind clean

all: $(BUILD_DIR)/editor_core_demo $(BUILD_DIR)/test_editor_core

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/gap_buffer.o: src/gap_buffer.c include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_core.o: src/editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_editor_core.o: tests/test_editor_core.c include/editor_core.h include/gap_buffer.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/editor_core_demo: $(BUILD_DIR)/main.o $(CORE_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_editor_core: $(BUILD_DIR)/test_editor_core.o $(CORE_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(BUILD_DIR)/test_editor_core
	./$(BUILD_DIR)/test_editor_core

valgrind: $(BUILD_DIR)/test_editor_core
	valgrind --leak-check=full --error-exitcode=1 ./$(BUILD_DIR)/test_editor_core

clean:
	rm -rf $(BUILD_DIR)
