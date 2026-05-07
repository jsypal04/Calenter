/* 
 * config.c
 *
 * This file provides a function for reading a config file
 * into a config type. The config file is located at ~/.config/calenter/config.
 * It uses the following basic syntax:
 *
 * key=value
 * */

#include "config.h"
#include "../calenter.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


void config_exists(char* dir);

Config read_config() {
    Config config = {0};

    char* home = getenv("HOME");
    char* config_path = malloc(sizeof(char) * (strlen(home) + strlen(CONFIG_DIR) + strlen(CONFIG_FILE) + 5));
    memset(config_path, '\0', sizeof(char) * (strlen(home) + strlen(CONFIG_DIR) + strlen(CONFIG_FILE) + 5));
    sprintf(config_path, "%s%s%s", home, CONFIG_DIR, CONFIG_FILE);

    FILE* config_file = fopen(config_path, "r");
    if (!config_file) return config;

    free(config_path);

    char* line = NULL;
    size_t len = 0;
    int read;

    do {
        read = getline(&line, &len, config_file);

        
        if (strstr(line, "remote_url")) {
            config.remote_url = strdup(line + strlen("remote_url") + 1);
            trim(config.remote_url);
        }
    } while (read > 0);

    free(line);
    fclose(config_file);

    return config;
}

