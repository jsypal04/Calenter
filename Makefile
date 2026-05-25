CC = gcc
CFLAGS = -g -Wall $(shell pkg-config --cflags --libs glib-2.0 libnotify)
LDLIBS = -lncurses -lcurl
BUILD_DIR = build

# SRC_FILES := $(shell find ./src -name "*.c")
# OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

LIB_SRC_FILES := $(shell find ./src/common -name "*.c" | sed 's#^\./##')
LIB_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(LIB_SRC_FILES))

BIN_SRC_FILES := $(shell find ./src/calenter -name "*.c" | sed 's#^\./##')
BIN_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(BIN_SRC_FILES))

NOTI_SRC_FILES := $(shell find ./src/notifications -name "*.c" | sed 's#^\./##')
NOTI_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(NOTI_SRC_FILES))

LIB  = $(BUILD_DIR)/libcalenter.so
BIN  = $(BUILD_DIR)/calenter
NOTI = $(BUILD_DIR)/calenter-notification-daemon

all: binary daemon

$(LIB): $(LIB_OBJ_FILES)
	@echo -e "Linking C shared library $(LIB)"
	@$(CC) $(CFLAGS) -shared -fPIC $(LIB_OBJ_FILES) -o $(LIB)
	@echo -e "\e[32mBuilt target $(LIB)\e[0m"

$(BIN): $(LIB) $(BIN_OBJ_FILES)
	@echo -e "Linking C executable $(BIN)"
	@$(CC) $(CFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) -o $(BIN) $(PWD)/$(LIB)
	@echo -e "\e[32mBuilt target $(BIN)\e[0m"

$(NOTI): $(LIB) $(NOTI_OBJ_FILES)
	@echo -e "Linking C executable $(NOTI)"
	@$(CC) $(CFLAGS) $(NOTI_OBJ_FILES) -o $(NOTI) $(PWD)/$(LIB)
	@echo -e "\e[32mBuilt target $(NOTI)\e[0m"

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

library: $(LIB)

binary: $(BIN)

daemon: $(NOTI)

clean:
	rm -rf $(BUILD_DIR)

install: $(BIN)
	cp $(BIN) $(HOME)/.local/bin


.PHONY: all library binary daemon clean install
