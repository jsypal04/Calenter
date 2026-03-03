#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sync.h"

#define SYNC_SCRIPT "/.calendar/scripts/fetch_calendar.bash"

char* get_url();
char* get_sync_script_path();

int sync_calendar() {

    char* url = get_url();
    char* sync_script_path = get_sync_script_path();

    if (sync_script_path == NULL) return NULL;
    
    if (fork() == 0) {
        // execl(sync_script_path, , ...);
    }

    return 0;
}

char* get_sync_script_path() {
    char* home = getenv("HOME");

    if (home == NULL) return NULL;

    int length = strlen(home) + strlen(SYNC_SCRIPT) + 5;

    char* sync_script_path = malloc(length * sizeof(char));
    memset(sync_script_path, '\0', length * sizeof(char));
    sprintf(sync_script_path, "%s%s", home, SYNC_SCRIPT);

    return sync_script_path;
}

