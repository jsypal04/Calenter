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
    char* calendartxt_path;
} Flags;

void  print_usage();
bool  is_valid_int(char* str);
Flags parse_cli_flags(int* argc, char** argv);
int parse_date(char* date, int* year, int* month, int* day);

int update_calendartxt(char* ics_file);

int main(int argc, char** argv) {
    Flags cli_flags = parse_cli_flags(&argc, argv);
    
    if (cli_flags.calendartxt_path != NULL) {
        set_calendar_path(cli_flags.calendartxt_path);
    }

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

        int year;
        int month;
        int day;

        int retval = parse_date(argv[2], &year, &month, &day);
        if (retval != 0) {
            return retval;
        }

        Array* events = get_events_str(year, month, day);

        for (int i = 0; i < array_len(events); i++) {
            char* event = get_string(events, i);
            printf("%s\n", event);
        }

        free_array(events);
    } else if (strcmp(argv[1], "add-event")) {
        if (argc != 3) {
            printf("Usage: calenter-headless add-event yyyy-mm-dd hh:mm <summary> <RRULE>");
            return 1;
        }

        int year;
        int month;
        int day;

        int retval = parse_date(argv[2], &year, &month, &day);
        if (retval != 0) {
            return retval;
        }

        
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
    printf("Usage: %s [-r | --redirect-logs] [-c | --calendartxt-path <path>] <command> [<args>]\n\n", BIN);
    printf("These are the available commands:\n\n");
    printf("  sync              updates calendar.txt with events from all remote_urls\n");
    printf("  get-events        prints the events for the provided date (yyyy-mm-dd)\n");
    printf("  update-calendar   updates calendar.txt using the provided ics file\n");
    printf("  parse-ics         prints the list of events in the provided ics file\n\n");
    printf("Flags:\n\n");
    printf("  -r, --redirect-logs             redirects debug_log calls to stdout instead of debug.log\n");
    printf("  -c, --calendartxt-path <path>   the calendar.txt file to use\n\n");
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

    int i = 0;
    while (i < *argc) {
        if (strlen(argv[i]) == 0) continue;
        if (argv[i][0] != '-') {
            new_argv[num_args] = argv[i];
            num_args++;
            i++;
            continue;
        }
        
        if (
            strcmp(argv[i], "-r") == 0 || 
            strcmp(argv[i], "--redirect-logs") == 0
        ) {
            cli_flags.redirect_logs = true;
        } else if (
            (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--calendartxt-path") == 0) &&
            i < *argc - 1
        ) {
            cli_flags.calendartxt_path = argv[i + 1];
            i++;
        } else {
            print_usage();
            _exit(EXIT_FAILURE);
        }
        i++;
    }

    *argc = num_args;
    for (int i = 0; i < num_args; i++) {
        argv[i] = new_argv[i];
    }

    free(new_argv);
    new_argv = NULL;

    return cli_flags;
}

int parse_date(char* date, int* year, int* month, int* day) {

    char year_str[5] = "\0";
    char month_str[3] = "\0";
    char day_str[3] = "\0";

    strncpy(year_str, date, 4);
    strncpy(month_str, date + 5, 2);
    strncpy(day_str, date + 8, 2);

    if (!is_valid_int(year_str) || !is_valid_int(month_str) || !is_valid_int(day_str)) {
        printf("Only numbers please!\n");
        return 1;
    }

    *year = atoi(year_str);
    *month = atoi(month_str);
    *day = atoi(day_str);

    return 0;
}
