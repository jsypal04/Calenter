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
HEADLESS = $(BUILD_DIR)/calenter-headless

all: $(BIN) $(LIB) $(HEADLESS)

library: $(LIB)

binary: $(BIN)

headless: $(HEADLESS)

run: $(BIN)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(BIN)

run-headless: $(HEADLESS)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(BUILD_DIR)/calenter-headless

clean:
	rm -rf $(BUILD_DIR)/*

$(LIB): $(LIB_OBJ_FILES)
	@echo -e "Linking C shared library $(LIB)"
	@$(CC) $(CFLAGS) -shared -fPIC $(LIB_OBJ_FILES) -o $(LIB)
	@echo -e "\e[32mBuilt target $(LIB)\e[0m"

$(BIN): $(LIB) $(BIN_OBJ_FILES)
	@echo -e "Linking C executable $(BIN)"
	@$(CC) $(CFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) \
	    -L$(BUILD_DIR) -lcalenter -o $(BIN)
	@echo -e "\e[32mBuilt target $(BIN)\e[0m"

$(BUILD_DIR)/src/calenter/%.o: src/calenter/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR)/src/common/%.o: src/common/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(HEADLESS): $(LIB)
	@mkdir -p $(BUILD_DIR)/src/headless
	@echo -e "Compiling and Linking C executable $(HEADLESS)"
	@$(CC) $(CFLAGS) $(LDLIBS) src/headless/calenter-headless.c \
	    -L$(BUILD_DIR) -lcalenter -o $(HEADLESS)
	@echo -e "\e[32mBuilt target $(HEADLESS)\e[0m"

.ONESHELL:
install: $(BIN)
	@mkdir -p $(APP_HOME)/.calendar

	echo "Installing shared object to /usr/lib"
	cp $(LIB) /usr/lib
	ldconfig

	if [ -f "$(APP_HOME)/.calendar/calendar.txt" ]; then
		echo "$(APP_HOME)/.calendar/calendar.txt already exists. Using it."
	else
		echo "$(APP_HOME)/.calendar/calendar.txt not found. Downloading a template."
		curl https://terokarvinen.com/2021/calendar-txt/calendar-txt-until-2033.txt > $(APP_HOME)/.calendar/calendar.txt
	fi

	echo "Installing binary to $(APP_HOME)/.local/bin"
	cp $(BIN) $(APP_HOME)/.local/bin
	echo "Installing python scripts to $(APP_HOME)/.calendar/scripts"
	cp -r scripts $(APP_HOME)/.calendar

	if ! command -v python3 >/dev/null 2>&1; then
		echo "Python 3 is not installed. You must install Python 3 to run sync calendar.txt with your Google Calendar."
	fi   

.PHONY: all library binary run clean install headless run-headless
