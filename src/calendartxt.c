/*
 * calendartxt.c
 *
 * This file is a driver for interacting with calendar.txt. The
 * two main functions are get_events (for getting the event of
 * a given day) and add_event (NOT YET IMPLEMENTED).
 *
 * TODO: Handle ALL DAY events
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "calenter.h"

#define CALENDAR_TXT "/.calendar/calendar.txt"

struct event parse_event(char* raw_event);

void format_time(char* buffer, int hour, int min) {
    if (hour == -1) {
        sprintf(buffer, "ALL DAY");
    } else if (hour < 10 && min < 10) {
        sprintf(buffer, "0%d:0%d", hour, min);
    } else if (hour < 10 && min >= 10) {
        sprintf(buffer, "0%d:%d", hour, min);
    } else if (hour >= 10 && min < 10) {
        sprintf(buffer, "%d:0%d", hour, min);
    } else if (hour >= 10 && min >= 10) {
        sprintf(buffer, "%d:%d", hour, min);
    }
}

void format_calendartxt_date(char* buffer, int year, int month, int day) {
    if (month >= 10 && day >= 10) {
        sprintf(buffer, "%d-%d-%d", year, month, day);
    } else if (month < 10 && day >= 10) {
        sprintf(buffer, "%d-0%d-%d", year, month, day);
    } else if (month >= 10 && day < 10) {
        sprintf(buffer, "%d-%d-0%d", year, month, day);
    } else if (month < 10 && day < 10) {
        sprintf(buffer, "%d-0%d-0%d", year, month, day);
    }
}

/**
 * Returns the events for the given date
 *
 * Note: month and day are not 0 indexed
 * TODO: May want to validate the input to this function.
 */
struct events get_events(int year, int month, int day) {
    char search_str[20];
    format_calendartxt_date(search_str, year, month, day);

    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        exit(1);
    }

    int length = strlen(home_dir) + strlen(CALENDAR_TXT);
    char* calendar_path = malloc(sizeof(char) * length);
    strcpy(calendar_path, home_dir);
    strcat(calendar_path, CALENDAR_TXT);

    FILE* calendar_file = fopen(calendar_path, "r");
    free(calendar_path);
    calendar_path = NULL;

    char* line = NULL;
    size_t len = 0;
    int read;

    do {
        read = getline(&line, &len, calendar_file);
    } while (strstr(line, search_str) == NULL || read < 19);

    if (read < 20) {
        // There are no events on this day.
        struct events events = {0};
        return events;
    }

    char* trimmed_line = malloc(sizeof(char) * read);
    bzero(trimmed_line, sizeof(char) * read);
    strcpy(trimmed_line, line + 20);
    free(line);
    line = NULL;

    struct events events;
    init_events(&events);

    char* token = strtok(trimmed_line, ",");
    while (token != NULL) {
        // printf("%s\n", token);
        struct event event = parse_event(token);
        event.year = year;
        event.month = month;
        event.day = day;

        append_event(&events, event);
        token = strtok(NULL, ",");
    }
    free(trimmed_line);
    fclose(calendar_file);
    trimmed_line = NULL;

    return events;
}

/*
 * Parses the string event from calendar.txt into a `struct event`
 * This function allocates memory for the event.
 */
struct event parse_event(char* raw_event) {
    struct event event = {0};
    int index = 0;

    if (strstr(raw_event, ":") != NULL) {
        while (raw_event[index] != ':') {
            index++;
        }
        char hour[3] = "\0";
        hour[0] = raw_event[index - 2];
        hour[1] = raw_event[index - 1];

        char min[3] = "\0";
        min[0] = raw_event[index + 1];
        min[1] = raw_event[index + 2];

        event.hour = atoi(hour);
        event.min = atoi(min);

        index += 3;
    } else {
        event.hour = -1;
        event.min = -1;

        while (raw_event[index] != '-') {
            index++;
        }
    }

    while (raw_event[index] == '-' || raw_event[index] == ' ') {
        index++;
    }

    event.summary = strdup(raw_event + index);
    return event;
}

void append_event(struct events* events, struct event new_event) {
    if (events->length == events->size) {
        struct event* longer_events = malloc(2 * events->size * sizeof(struct event));

        // printf("events->length = %zu\n", events->length);
        for (int i = 0; i < events->length; i++) {
            longer_events[i] = events->events[i];
            // printf("running line %d\n", __LINE__);
            free(events->events[i].summary);
        }
        // printf("running line %d\n", __LINE__);
        free(events->events);

        events->size *= 2;
        events->events = longer_events;
    }

    events->events[events->length] = new_event;
    events->length++;
}

void init_events(struct events* events) {
    events->length = 0;
    events->size = 10;
    events->events = malloc(events->size * sizeof(struct event)); // Hitting an error here
}
