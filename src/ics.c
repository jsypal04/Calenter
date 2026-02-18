/*
 * ics.c
 *
 * This file contains functions to parse events in a ICS
 * file into data structures that can be written to calendat.txt
 * using the functions in calendartxt.c.
 *
 * NOTE: This file does not work yet and is not actually compiled and
 * linked into the project yet.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "calenterm.h"

#define INIT_EVENTS_SIZE 500


struct argument {
    char* key;
    char* value;
};

struct property {
    char* value;
    size_t num_key_args;
    size_t num_value_args;
    struct argument* key_args;
    struct argument* value_args;
};

// TODO: Handle timestamps without hours and minutes
void parse_ISO8601_timestamp(char* timestamp, struct event* event) {
    event->year = 0;
    event->month = 0;
    event->day = 0;
    event->hour = 0;
    event->min = 0;

    sscanf(timestamp, "%4d%2d%2dT%2d%2dZ",
               &event->year, &event->month, &event->day,
               &event->hour, &event->min);

}

void free_property_data(struct property property) {
    if (property.key_args != NULL) {
        for (int i = 0; i < property.num_key_args; i++) {
            free(property.key_args[i].key);
            free(property.key_args[i].value);
        }
        free(property.key_args);
    }

    if (property.value_args != NULL) {
        for (int i = 0; i < property.num_value_args; i++) {
            free(property.value_args[i].key);
            free(property.value_args[i].value);
        }
        free(property.value_args);
    }

    if (property.value != NULL) {
        free(property.value);
    }
}

struct property parse_event_property(char* line, size_t length) {
    size_t current_index = 0;

    struct property property;

    while (line[current_index] != ';' && line[current_index] != ':') {
        current_index++;
    }

    if (line[current_index] == ';') {
        size_t argument_index = 0;
        property.num_key_args = 0;
        struct argument* key_args = malloc(50 * sizeof(struct argument));

        while (line[current_index] == ';') {
            current_index++;
            size_t arg_start = current_index;
            while (line[current_index] != '=') {
                current_index++;
            }

            key_args[argument_index].key = strndup(line + arg_start, current_index - arg_start);

            current_index++;
            arg_start = current_index;
            while (line[current_index] != ';' && line[current_index] != ':') {
                current_index++;
            }

            key_args[argument_index].value = strndup(line + arg_start, current_index - arg_start);

            argument_index++;
            property.num_key_args++;
        }
        property.key_args = key_args;
    } else {
        property.key_args = NULL;
        property.num_key_args = 0;
    }

    if (strstr(line, "RRULE") != NULL) {
        property.value = NULL;
    } else {
        current_index++;
        size_t value_start = current_index;

        while (line[current_index] != ';' && line[current_index] != '\n') {
            current_index++;
        }

        property.value = strndup(line + value_start, current_index - value_start);
    }
    if (line[current_index] == '\n') {
        property.value_args = NULL;
        return property;
    }

    if (line[current_index] == ';' || line[current_index] == ':') {
        size_t argument_index = 0;
        property.num_value_args = 0;
        struct argument* value_args = malloc(50 * sizeof(struct argument));

        while (line[current_index] == ';' || line[current_index] == ':') {
            current_index++;
            size_t arg_start = current_index;
            while (line[current_index] != '=') {
                current_index++;
            }

            value_args[argument_index].key = strndup(line + arg_start, current_index - arg_start);

            current_index++;
            arg_start = current_index;
            while (line[current_index] != ';' && line[current_index] != '\n') {
                current_index++;
            }

            value_args[argument_index].value = strndup(line + arg_start, current_index - arg_start);

            argument_index++;
            property.num_value_args++;
        }
        property.value_args = value_args;
    } else {
        property.value_args = NULL;
        property.num_value_args = 0;
    }

    return property;
}

struct event parse_ics_event(FILE* ics_file) {
    struct event event;

    char* line = NULL;
    size_t len = 0;
    size_t read = getline(&line, &len, ics_file);

    while (strstr(line, "DTSTART") == NULL) {
        read = getline(&line, &len, ics_file);
    }
    struct property start_property = parse_event_property(line, read);
    parse_ISO8601_timestamp(start_property.value, &event);
    printf("%d-%d-%d %d:%d\n", event.year, event.month, event.day, event.hour, event.min);
    free_property_data(start_property);

    while (strstr(line, "RRULE") == NULL && strstr(line, "SUMMARY") == NULL) {
        read = getline(&line, &len, ics_file);
    }
    if (strstr(line, "RRULE") != NULL) {
        struct property rrule_property = parse_event_property(line, read);
        free_property_data(rrule_property);
    }

    while (strstr(line, "SUMMARY") == NULL) {
        read = getline(&line, &len, ics_file);
    }

    struct property summary_property = parse_event_property(line, read);
    event.summary = summary_property.value;
    // free_property_data(summary_property);

    return event;
}

void parse_ics(char* ics_file_path) {
    FILE* ics_file = fopen(ics_file_path, "r");
    if (ics_file == NULL) {
        perror("Failed to open ICS file");
        return;
    }

    char* line = NULL;
    size_t len = 0;
    size_t read;

    struct events events = {
        INIT_EVENTS_SIZE,
        0,
        malloc(INIT_EVENTS_SIZE * sizeof(struct event)),
    };

    while ((read = getline(&line, &len, ics_file)) != -1) {

        if (strcmp(line, "BEGIN:VEVENT\r\n") == 0) {
            struct event event = parse_ics_event(ics_file);
            // printf("%zu-%zu-%zu %zu:%zu - %s\n", event.year, event.month, event.day, event.hour, event.min, event.summary);
            append_event(&events, event);
        }
    }

    printf("number of events: %zu\n", events.length);

    fclose(ics_file);
    free(line);
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./ics path/to/ics/file\n");
    }

    parse_ics(argv[1]);

    return 0;
}
