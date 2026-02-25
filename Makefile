CC = gcc
CFLAGS = -g -Wall
LDLIBS = -lncurses
BUILD_DIR = build

SRC_FILES := $(shell find . -name "*.c" ! -name "ics.c")
OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

BIN = $(BUILD_DIR)/calenter

$(BIN): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(BIN) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
