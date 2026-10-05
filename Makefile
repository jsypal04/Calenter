APP_USER=$(shell stat -c '%U' .)
APP_HOME=$(shell getent passwd $(APP_USER) | cut -d: -f6)

MAJOR = 0
MINOR = 3
PATCH = 1

# TESTING = -DTESTING

CC = gcc
CFLAGS = -g -Wall -Iinclude $(shell pkg-config --cflags --libs glib-2.0 libnotify) $(TESTING)
LDLIBS = -lncurses -lcurl -lm -pthread
BUILD_DIR = build

CXX = g++
CXXFLAGS = $(CFLAGS)

LIB_SRC_FILES := $(shell find src/common -type f -name "*.c")
LIB_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(LIB_SRC_FILES))

BIN_CPP_FILES := $(shell find src/calenter -type f -name "*.cpp")
BIN_C_FILES := $(shell find src/calenter -type f -name "*.c")
BIN_C_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(BIN_C_FILES))
BIN_CPP_OBJ_FILES := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(BIN_CPP_FILES))
BIN_OBJ_FILES := $(BIN_C_OBJ_FILES)$(BIN_CPP_OBJ_FILES)

TEST_SRC_FILES := $(shell find tests -type f -name "*.c")
TEST_OBJ_FILES := $(patsubst %.c, $(BUILD_DIR)/%.o, $(TEST_SRC_FILES))

HEADLESS_OBJ_FILES := $(BUILD_DIR)/src/headless/calenter-headless.o $(BUILD_DIR)/src/calenter/json.o $(BUILD_DIR)/src/calenter/cpp/channel.o

LIB_NAME      = libcalenter.so
LIB_REAL_NAME = $(LIB_NAME).$(MAJOR).$(MINOR).$(PATCH)
SONAME        = $(LIB_NAME).$(MAJOR)

LIB  = $(BUILD_DIR)/$(LIB_REAL_NAME)
BIN  = $(BUILD_DIR)/calenter
HEADLESS = $(BUILD_DIR)/calenter-headless
TEST_BIN = $(BUILD_DIR)/calenter-tests

all: $(BIN) $(LIB) $(HEADLESS)

library: $(LIB)

binary: $(BIN)

headless: $(HEADLESS)

test: $(TEST_BIN)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(TEST_BIN)

run: $(BIN)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(BIN)

run-headless-gdb: $(HEADLESS)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	gdb $(BUILD_DIR)/calenter-headless

run-headless: $(HEADLESS)
	@LD_LIBRARY_PATH=$(PWD)/$(BUILD_DIR) \
	./$(BUILD_DIR)/calenter-headless

clean:
	rm -rf $(BUILD_DIR)/*

$(LIB): $(LIB_OBJ_FILES)
	@echo -e "Linking C shared library $(LIB)"
	@$(CXX) $(CXXFLAGS) -shared -fPIC $(LIB_OBJ_FILES) -o $(LIB)
	@echo -e "\e[32mBuilt target $(LIB)\e[0m"
	@ln -sf $(LIB_REAL_NAME) $(BUILD_DIR)/$(SONAME)
	@ln -sf $(SONAME) $(BUILD_DIR)/$(LIB_NAME)

$(BIN): $(LIB) $(BIN_OBJ_FILES)
	@echo -e "Linking C executable $(BIN)"
	@$(CXX) $(CXXFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) \
	    -L$(BUILD_DIR) -lcalenter -o $(BIN)
	@echo -e "\e[32mBuilt target $(BIN)\e[0m"

$(TEST_BIN): $(LIB) $(BIN_OBJ_FILES) $(TEST_OBJ_FILES)
	@echo -e "Linking C executable $(TEST_BIN)"
	@$(CXX) $(CXXFLAGS) $(LDLIBS) $(BIN_OBJ_FILES) $(TEST_OBJ_FILES) \
		-L$(BUILD_DIR) -lcalenter -o $(TEST_BIN)
	@echo -e "\e[32mBuilt target $(TEST_BIN)\e[0m"
	

$(BUILD_DIR)/src/calenter/%.o: src/calenter/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR)/src/calenter/cpp/%.o: src/calenter/cpp/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(BUILD_DIR)/src/common/%.o: src/common/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR)/tests/%.o: tests/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"

$(BUILD_DIR)/src/headless/%.o: src/headless/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "Compiling C object $@"
	

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(HEADLESS): $(LIB) $(HEADLESS_OBJ_FILES)
	@echo -e "Linking C executable $@"
	@$(CXX) $(CXXFLAGS) $(LDLIBS) $(HEADLESS_OBJ_FILES) \
	    -L$(BUILD_DIR) -lcalenter -o $(HEADLESS)
	@echo -e "\e[32mBuilt target $@\e[0m"

.ONESHELL:
install: $(BIN)
	mkdir -p $(APP_HOME)/.calendar

	echo "Installing shared object to /usr/lib"
	cp $(LIB) /usr/lib
	ln -sf /usr/lib/$(LIB_REAL_NAME) /usr/lib/$(SONAME)
	ln -sf /usr/lib/$(SONAME) /usr/lib/$(LIB_NAME)
	ldconfig

	if [ -f "$(APP_HOME)/.calendar/calendar.txt" ]; then
		echo "$(APP_HOME)/.calendar/calendar.txt already exists. Using it."
	else
		echo "$(APP_HOME)/.calendar/calendar.txt not found. Downloading a template."
		curl https://terokarvinen.com/2021/calendar-txt/calendar-txt-until-2033.txt > $(APP_HOME)/.calendar/calendar.txt
	fi

	echo "Installing binary to $(APP_HOME)/.local/bin"
	cp $(BIN) $(APP_HOME)/.local/bin

.PHONY: all library binary run clean install headless run-headless
