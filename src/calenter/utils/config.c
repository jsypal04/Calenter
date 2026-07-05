/*
 * config.c
 *
 * This file provides a function for reading a config file
 * into a config type. The config file is located at ~/.config/calenter/config.
 * It uses the following basic syntax:
 *
 * key=value
 * */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "debug.h"
#include "../calenter.h"

typedef struct line {
    char key[64];
    char* value;
} ConfigLine;

int retval;

ConfigLine parse_next_config_line(FILE* config_file);
bool contains(const char** keys, char* key);

Config read_config() {
    Config config = {0};

    char* home = getenv("HOME");
    char* config_path = malloc(sizeof(char) * (strlen(home) + strlen(CONFIG_DIR) + strlen(CONFIG_FILE) + 5));
    memset(config_path, '\0', sizeof(char) * (strlen(home) + strlen(CONFIG_DIR) + strlen(CONFIG_FILE) + 5));
    sprintf(config_path, "%s%s%s", home, CONFIG_DIR, CONFIG_FILE);

    FILE* config_file = fopen(config_path, "r");
    if (!config_file) return config;

    free(config_path);

    ConfigLine line;

    line = parse_next_config_line(config_file);
    while (retval != EOF) {
        if (strcmp(line.key, "remote_url") == 0) {
            config.remote_url = strdup(line.value);
            debug_log("remote_url: %s\n", config.remote_url);
        } else if (strcmp(line.key, "enable_notifications") == 0) {
            if (
                strcmp(line.value, "true") == 0 ||
                strcmp(line.value, "True") == 0 ||
                strcmp(line.value, "TRUE") == 0
            ) {
                config.enable_notifications = true;
                debug_log("enable_notifications: true\n");
            } else {
                config.enable_notifications = false;
                debug_log("enable_notifications: false\n");
            }
        } else if (strcmp(line.key, "notify_time") == 0) {
            config.notify_time = atoi(line.value);
            debug_log("notify_time: %d\n", config.notify_time);
        } else {
            debug_log("Bad config option: '%s'\n", line.key);
        }

        free(line.value);
        line.value = NULL;

        line = parse_next_config_line(config_file);
    }

    if (config.enable_notifications && config.notify_time == 0) {
        config.notify_time = 10;
    }

    fclose(config_file);

    return config;
}

ConfigLine parse_next_config_line(FILE* config_file) {
    ConfigLine config_line = {0};
    char* line = NULL;
    size_t len = 0;
    int read;

    read = getline(&line, &len, config_file);
    if (read <= 0) {
        retval = EOF;
        free(line);
        line = NULL;
        return config_line;
    }

    char* eq_ptr = strstr(line, "=");
    if (eq_ptr == NULL) {
        retval = 1;
        free(line);
        line = NULL;
        return config_line;
    }
    int key_len = eq_ptr - line;

    assert(key_len < 64);

    strncpy(config_line.key, line, key_len);
    trim(config_line.key);

    config_line.value = strdup(eq_ptr + 1);
    trim(config_line.value);

    retval = 0;
    return config_line;
}
