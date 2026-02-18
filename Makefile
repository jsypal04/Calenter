CC = gcc
CFLAGS = -g
LDLIBS = -lncurses
BUILD_DIR = build

OBJ_FILES = $(BUILD_DIR)/main.o $(BUILD_DIR)/calendartxt.o
BIN = $(BUILD_DIR)/main

$(BIN): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(BUILD_DIR)/main $(LDLIBS)

$(BUILD_DIR)/main.o: $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/calendartxt.o: $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/calendartxt.c -o $(BUILD_DIR)/calendartxt.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
