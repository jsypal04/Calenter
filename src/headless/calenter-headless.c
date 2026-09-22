#include <libnotify/notify.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "../common/config.h"
#include "../common/sync.h"
#include "../common/array.h"
#include "../common/calendartxt.h"
#include "../common/ics.h"


#define BIN "calenter-headless"

extern bool headless;
extern pthread_t syncer_thread;

typedef struct flags {
    bool redirect_logs;
} Flags;

void  print_usage();
bool  is_valid_int(char* str);
Flags parse_cli_flags(int* argc, char** argv);

int update_calendartxt(char* ics_file);

int main(int argc, char** argv) {
    Flags cli_flags = parse_cli_flags(&argc, argv);

    notify_init("Calenter");

    if (cli_flags.redirect_logs) {
        headless = true;
    } else {
        headless = false;
    }

    if (argc < 2) {
        print_usage();
        return 1;
    }

    Config config = read_config();
    
    if (strcmp(argv[1], "sync") == 0) {
        sync_calendar_wrapper(config.remote_urls);
        pthread_join(syncer_thread, NULL);
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

        Array* events = get_events_str(atoi(year), atoi(month), atoi(day));

        for (int i = 0; i < array_len(events); i++) {
            char* event = get_string(events, i);
            printf("%s\n", event);
        }

        free_array(events);
    } else if (strcmp(argv[1], "update-calendar") == 0) {
        if (argc != 3) {
            printf("Usage: calenter-headless update-calendar path/to/calendar_file.ics\n");
            return 1;
        }

        update_calendartxt(argv[2]);
    } else if (strcmp(argv[1], "parse-ics") == 0) {
        if (argc != 3) {
            printf("Usage: calendter-headless parse-ics path/to/calendar_file.ics\n");
            return 1;
        }

        parse_ics(argv[2]);
    } else {
        print_usage();
    }

    free_array(config.remote_urls);

    notify_uninit();

    return 0;
}

void print_usage() {
    printf("Usage: %s [-r | --redirect-logs] <command> [<args>]\n\n", BIN);
    printf("These are the available commands:\n\n");
    printf("  sync              updates calendar.txt with events from all remote_urls\n");
    printf("  get-events        prints the events for the provided date (yyyy-mm-dd)\n");
    printf("  update-calendar   updates calendar.txt using the provided ics file\n");
    printf("  parse-ics         prints the list of events in the provided ics file\n\n");
    printf("Flags:\n\n");
    printf("  -r, --redirect-logs   redirects debug_log calls to stdout instead of debug.log\n\n");
}

bool is_valid_int(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] < 48 || str[i] > 57) return false;
    }
    return true;
}

Flags parse_cli_flags(int* argc, char** argv) {
    Flags cli_flags = {0};

    int num_args = 0;
    char** new_argv = malloc(sizeof(char*) * (*argc));

    for (int i = 0; i < *argc; i++) {
        if (strlen(argv[i]) == 0) continue;
        if (argv[i][0] != '-') {
            new_argv[num_args] = argv[i];
            num_args++;
            continue;
        }
        
        if (
            strcmp(argv[i], "-r") == 0 || 
            strcmp(argv[i], "--redirect-logs") == 0
        ) {
            cli_flags.redirect_logs = true;
        } else {
            print_usage();
            _exit(EXIT_FAILURE);
        }

    }

    *argc = num_args;
    for (int i = 0; i < num_args; i++) {
        argv[i] = new_argv[i];
    }

    free(new_argv);
    new_argv = NULL;

    return cli_flags;
}
