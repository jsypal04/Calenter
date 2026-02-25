CC = gcc
CFLAGS = -g -Wall
LDLIBS = -lncurses
BUILD_DIR = build

OBJ_FILES = $(BUILD_DIR)/calenter.o $(BUILD_DIR)/modal.o $(BUILD_DIR)/calendartxt.o
BIN = $(BUILD_DIR)/calenter

$(BIN): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(BIN) $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
