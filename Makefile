CC = gcc
CFLAGS = -g -Wall $(shell pkg-config --cflags --libs glib-2.0 libnotify)
LDLIBS = -lncurses -lcurl
BUILD_DIR = build

# SRC_FILES := $(shell find ./src -name "*.c")
# OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

LIB_SRC_FILES := $(shell find ./src/common -name "*.c")
LIB_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(LIB_SRC_FILES))

BIN_SRC_FILES := $(shell find ./src/calenter -name "*.c")
BIN_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(BIN_SRC_FILES))

NOTI_SRC_FILES := $(shell find ./src/notifications -name "*.c")
NOTI_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(NOTI_SRC_FILES))

LIB  = $(BUILD_DIR)/libcalenter.so
BIN  = $(BUILD_DIR)/calenter
NOTI = $(BUILD_DIR)/calenter-notification-daemon

all: binary daemon

$(LIB): $(LIB_OBJ_FILES)
	@echo -e "\e[32mLinking shared object...\e[0m"
	$(CC) $(CFLAGS) -shared -fPIC $(LIB_OBJ_FILES) -o $(LIB)

$(BIN): $(LIB) $(BIN_OBJ_FILES)
	$(CC) $(CFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) -o $(BIN) $(PWD)/$(LIB)

$(NOTI): $(LIB) $(NOTI_OBJ_FILES)
	$(CC) $(CFLAGS) $(NOTI_OBJ_FILES) -o $(NOTI) $(PWD)/$(LIB)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

library: $(LIB)

binary: $(BIN)

daemon: $(NOTI)

clean:
	rm -rf $(BUILD_DIR)

install: $(BIN)
	cp $(BIN) $(HOME)/.local/bin


.PHONY: all library binary daemon clean install
