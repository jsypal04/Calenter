CC = gcc
CFLAGS = -g
LDLIBS = -lncurses
BUILD_DIR = build

OBJ_FILES = $(BUILD_DIR)/main.o $(BUILD_DIR)/calendartxt.o

main: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(BUILD_DIR)/main $(LDLIBS)

$(BUILD_DIR)/main.o: main.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/calendartxt.o: calendartxt.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c calendartxt.c -o $(BUILD_DIR)/calendartxt.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
