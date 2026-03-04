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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Config read_config() {
    Config config = {0};

    char* home = getenv("HOME");
    char* abs_path = malloc(sizeof(char) * (strlen(home) + strlen(CONFIG_PATH) + 5));
    memset(abs_path, '\0', sizeof(char) * (strlen(home) + strlen(CONFIG_PATH) + 5));
    sprintf(abs_path, "%s%s", home, CONFIG_PATH);

    FILE* config_file = fopen(abs_path, "r");

    free(abs_path);

    if (config_file == NULL) return config;

    char* line = NULL;
    size_t len = 0;
    int read;

    do {
        read = getline(&line, &len, config_file);

        
        if (strstr(line, "remote_url")) {
            config.remote_url = strdup(line + strlen("remote_url") + 1);
            if (config.remote_url[strlen(config.remote_url) - 1] == '\n') {
                config.remote_url[strlen(config.remote_url) - 1] = '\0';
            }
        }
    } while (read > 0);

    free(line);
    fclose(config_file);

    return config;
}
