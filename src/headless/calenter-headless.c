#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/config.h"
#include "../common/sync.h"
#include "../common/array.h"
#include "../common/calendartxt.h"


void print_usage();
bool is_valid_int(char* str);

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    Config config = read_config();
    
    if (strcmp(argv[1], "sync") == 0) {
        sync_calendar_wrapper(config.remote_urls);
    } else if (strcmp(argv[1], "get-events") == 0) {
        if (argc != 3) {
            printf("Usage: calenter-headless get-events yyyy-mm-dd\n");
            return 1;
        }
        char year[5] = "\0";
        char month[3] = "\0";
        char day[3] = "\0";

        strncpy(year, argv[2], 4);
        strncpy(month, argv[2] + 5, 2);
        strncpy(day, argv[2] + 8, 2);

        if (!is_valid_int(year) || !is_valid_int(month) || !is_valid_int(day)) {
            printf("Only numbers please!\n");
            return 1;
        }

        int num_events;
        char** events = get_events_str(atoi(year), atoi(month), atoi(day), &num_events);

        for (int i = 0; i < num_events; i++) {
            printf("%s\n", events[i]);
        }
    }

    free_array(config.remote_urls);
    config.remote_urls = NULL;

    return 0;
}

void print_usage() {
    printf("Usage: calenter-headless <command> [<args>]\n");
}

bool is_valid_int(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] < 48 || str[i] > 57) return false;
    }
    return true;
}
