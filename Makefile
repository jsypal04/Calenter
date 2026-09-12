APP_USER=$(shell stat -c '%U' .)
APP_HOME=$(shell getent passwd $(APP_USER) | cut -d: -f6)

CC = gcc
CFLAGS = -Wall $(shell pkg-config --cflags --libs glib-2.0 libnotify)
LDLIBS = -lncurses -lcurl -lm
BUILD_DIR = build

LIB_SRC_FILES := $(shell find ./src/common -name "*.c" | sed 's#^\./##')
LIB_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(LIB_SRC_FILES))

BIN_SRC_FILES := $(shell find ./src/calenter -name "*.c" | sed 's#^\./##')
BIN_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(BIN_SRC_FILES))


LIB  = $(BUILD_DIR)/libcalenter.so
BIN  = $(BUILD_DIR)/calenter

all: binary daemon

$(LIB): $(LIB_OBJ_FILES)
	@echo -e "Linking C shared library $(LIB)"
	@$(CC) $(CFLAGS) -shared -fPIC $(LIB_OBJ_FILES) -o $(LIB)
	@echo -e "\e[32mBuilt target $(LIB)\e[0m"

$(BIN): $(LIB) $(BIN_OBJ_FILES)
	@echo -e "Linking C executable $(BIN)"
	@$(CC) $(CFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) \
	    -L$(BUILD_DIR) -lcalenter -o $(BIN)
	@echo -e "\e[32mBuilt target $(BIN)\e[0m"


$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

library: $(LIB)

binary: $(BIN)


run: $(BIN)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR)/*

install: $(BIN)
	mkdir -p $(APP_HOME)/.calendar

	cp $(LIB) /usr/lib
	ldconfig

	curl https://terokarvinen.com/2021/calendar-txt/calendar-txt-until-2033.txt > $(APP_HOME)/.calendar/calendar.txt

	cp $(BIN) $(APP_HOME)/.local/bin

	cp -r scripts $(APP_HOME)/.calendar


.PHONY: all library binary daemon run clean install
