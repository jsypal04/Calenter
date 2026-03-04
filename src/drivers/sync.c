/*
 * sync.c
 *
 * This file provides a single function for downloading
 * the ics file from the url specified in the config file.
 * Currently it uses the python/bash script setup that I
 * developed initially but I would like to migrate that over
 * to a C version.
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sync.h"
#include "config.h"

#define SYNC_SCRIPT "fetch_calendar.bash"
#define SYNC_SCRIPT_PATH "/.calendar/scripts/fetch_calendar.bash"

typedef enum _ERRNO {
    OK,
    NO_SYNC_SCRIPT_PATH,
    NO_REMOTE
} SYNC_ERR;

char* get_sync_script_path();

int sync_calendar() {

    Config config = read_config();
    if (config.remote_url == NULL) return NO_REMOTE;

    char* sync_script_path = get_sync_script_path();

    if (sync_script_path == NULL) return NO_SYNC_SCRIPT_PATH;

    if (fork() == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        execl(sync_script_path, SYNC_SCRIPT, config.remote_url, NULL);
    }

    free(config.remote_url);

    return 0;
}

/*
 * Gets the absolute path to the sync script
 * */
char* get_sync_script_path() {
    char* home = getenv("HOME");

    if (home == NULL) return NULL;

    int length = strlen(home) + strlen(SYNC_SCRIPT_PATH) + 5;

    char* sync_script_path = malloc(length * sizeof(char));
    memset(sync_script_path, '\0', length * sizeof(char));
    sprintf(sync_script_path, "%s%s", home, SYNC_SCRIPT_PATH);

    return sync_script_path;
}
