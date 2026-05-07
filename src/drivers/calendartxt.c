/*
 * calendartxt.c
 *
 * This file is a driver for interacting with calendar.txt. The
 * two main functions are get_events (for getting the event of
 * a given day).
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "calendartxt.h"

#define CALENDAR_TXT "/.calendar/calendar.txt"

void debug_log(const char *format, ...);

/*
 * Parses the string event from calendar.txt into a `struct event`
 * This function allocates memory for the event.
 */
struct event parse_event(char* raw_event);

char* get_calendar_path();
int write_events(struct events events, int year, int month, int day);
int remove_event(struct events* events, struct event event);
char* stringify_events(struct events events);

/**
 * Returns the events for the given date
 *
 * Note: month and day are not 0 indexed
 * TODO: May want to validate the input to this function.
 */
struct events get_events(int year, int month, int day) {
    char search_str[20];
    format_calendartxt_date(search_str, year, month, day);

    char* calendar_path = get_calendar_path();

    FILE* calendar_file = fopen(calendar_path, "r");
    free(calendar_path);
    calendar_path = NULL;

    char* line = NULL;
    size_t len = 0;
    int read;

    do {
        read = getline(&line, &len, calendar_file);
    } while (strstr(line, search_str) == NULL || read < 19);

    if (read <= 21) {
        // There are no events on this day.
        struct events events;
        init_events(&events);
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
        struct event event = parse_event(token);        

        event.datetime.tm_year = year - 1900;
        event.datetime.tm_mon = month - 1;
        event.datetime.tm_mday = day;
        event.datetime.tm_isdst = -1;
        mktime(&event.datetime);

        append_event(&events, event);
        token = strtok(NULL, ",");
    }
    free(trimmed_line);
    fclose(calendar_file);
    trimmed_line = NULL;

    return events;
}

struct event parse_event(char* raw_event) {
    struct event event = {0};
    struct tm datetime = {0};
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

        datetime.tm_hour = atoi(hour);
        datetime.tm_min = atoi(min);
        event.datetime = datetime;

        index += 3;
    } else {
        event.all_day = true;

        while (raw_event[index] != '-') {
            index++;
        }
    }

    while (raw_event[index] == '-' || raw_event[index] == ' ') {
        index++;
    }

    if (raw_event[strlen(raw_event) - 1] == '\n') {
        raw_event[strlen(raw_event) - 1] = '\0';
    }

    event.summary = strdup(raw_event + index);
    return event;
}

int delete_event(struct event event) {
    struct events events = get_events(
            event.datetime.tm_year + 1900, 
            event.datetime.tm_mon + 1, 
            event.datetime.tm_mday);

    remove_event(&events, event);
    if (
        write_events(
            events, event.datetime.tm_year + 1900, event.datetime.tm_mon + 1,
            event.datetime.tm_mday
        ) != 0
    ) return -1;

    return 0;
}

int add_event(struct event event, int year, int month, int day) {
    struct events events = get_events(year, month, day);
    insert_event(&events, event);

    if (write_events(events, year, month, day) != 0) return -1;

    return 0;
}

int write_events(struct events events, int year, int month, int day) {
    char search_str[20] = "\0";
    format_calendartxt_date(search_str, year, month, day);

    char* calendar_path = get_calendar_path();
    FILE* calendar_file = fopen(calendar_path, "r");
    FILE* tmp = fopen("tmp.txt", "w");

    if (calendar_file == NULL || tmp == NULL) return -1;

    char* line = NULL;
    size_t len;
    int read;

    char* str_events = stringify_events(events);

    do {
        read = getline(&line, &len, calendar_file);
        if (strstr(line, search_str) != NULL) {
            char header[30] = "\0";
            for (int i = 0; i < 18; i++) {
                header[i] = line[i];
            }

            fprintf(tmp, "%s  %s\n", header, str_events);

        } else {
            fprintf(tmp, "%s", line);
        }
    } while (read >= 0);

    fclose(calendar_file);
    fclose(tmp);
    remove(calendar_path);
    rename("tmp.txt", calendar_path);

    free(line);
    free(calendar_path);
    free(str_events);

    line = NULL;
    calendar_path = NULL;
    str_events = NULL;

    free_events(events);

    return 0;
}

char* stringify_events(struct events events) {
    int length = 100;
    for (int i = 0; i < events.length; i++) {
        struct event event = events.events[i];

        // Space for the time
        if (event.all_day) {
            length += 7;
        } else {
            length += 5;
        }

        // space for the hyphen
        length += 3;
        length += strlen(event.summary);
    }

    char* str_events = malloc(length * sizeof(char));
    memset(str_events, 0, length * sizeof(char));

    int write_index = 0;
    for (int i = 0; i < events.length; i++) {
        struct event event = events.events[i];

        if (write_index >= length) {
            // debug_log("Did not allocate enough space for events. Writing truncated events list.\n");
            return str_events;
        }
        char time[10] = "\0";
        if (!event.all_day) {
            format_time(time, event.datetime.tm_hour, event.datetime.tm_min);
        } else {
            sprintf(time, "ALL DAY");
        }
        if (i < events.length - 1) {
            sprintf(str_events + write_index, "%s - %s,", time, event.summary);
        } else {
            sprintf(str_events + write_index, "%s - %s", time, event.summary);
        }
        write_index = strlen(str_events);
    }

    return str_events;

}

int find_event(struct events* events, struct event event) {
    for (int i = 0; i < events->length; i++) {
        struct event cur_event = events->events[i];

        time_t cur_event_time = mktime(&cur_event.datetime);
        time_t event_time     = mktime(&event.datetime);

        if (cur_event_time != event_time) continue;
        if (strcmp(cur_event.summary, event.summary) != 0) continue;

        return i;
    }
    return -1;
}

int remove_event(struct events* events, struct event event) {
    int index = find_event(events, event);
    if (index < 0) return index;

    events->length--;
    for (int i = index; i < events->length; i++) {
        events->events[i] = events->events[i + 1];
    }

    return 0;
}

void insert_event(struct events* events, struct event new_event) {
    append_event(events, new_event);

    int index = events->length - 1;
    while (
        index > 0 &&
        time_cmp(
            new_event.datetime.tm_hour, 
            new_event.datetime.tm_min, 
            events->events[index - 1].datetime.tm_hour, 
            events->events[index - 1].datetime.tm_min
        ) < 0
    ) {
        struct event tmp = events->events[index - 1];
        events->events[index - 1] = events->events[index];
        events->events[index] = tmp;
        index--;
    }
}

void append_event(struct events* events, struct event new_event) {
    if (events->length == events->size) {
        struct event* longer_events = malloc(2 * events->size * sizeof(struct event));

        for (int i = 0; i < events->length; i++) {
            longer_events[i] = events->events[i];
        }
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
    events->events = malloc(events->size * sizeof(struct event));
}

void free_events(struct events events) {
    for (int i = 0; i < events.length; i++) {
        free(events.events[i].summary);
    }
    free(events.events);
}

int time_cmp(int hour1, int min1, int hour2, int min2) {
    if (hour1 < hour2) {
        return -1;
    } else if (hour1 > hour2) {
        return 1;
    }

    if (min1 < min2) {
        return -1;
    } else if (min1 > min2) {
        return 1;
    }

    return 0;
}

int date_cmp(int year1, int month1, int day1,
             int year2, int month2, int day2) {

    if (year1 < year2) {
        return -1;
    } else if (year1 > year2) {
        return 1;
    }

    if (month1 < month2) {
        return -1;
    } else if (month1 > month2) {
        return 1;
    }

    if (day1 < day2) {
        return -1;
    } else if (day1 > day2) {
        return 1;
    }

    return 0;
}

void format_time(char* buffer, int hour, int min) {
    if (hour < 10 && min < 10) {
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

char* get_calendar_path() {
    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        exit(1);
    }

    int length = strlen(home_dir) + strlen(CALENDAR_TXT) + 100;
    char* calendar_path = malloc(sizeof(char) * length);
    memset(calendar_path, 0, sizeof(char) * length);
    strcpy(calendar_path, home_dir);
    strcat(calendar_path, CALENDAR_TXT);

    return calendar_path;
}

struct tm get_last_date() {
    struct tm last_date = {0};

    char* calendar_path = get_calendar_path();
    FILE* calendar = fopen(calendar_path, "r");
    free(calendar_path);

    if (fseek(calendar, 0, SEEK_END) != 0) {
        fclose(calendar);
        return last_date;
    }

    long pos = ftell(calendar); 
    pos--;
    while (pos >= 0) {
         fseek(calendar, pos, SEEK_SET);

         int c = fgetc(calendar);

         if (c != '\n' && c != '\r') {
             break;
         }

         pos--;
    }

    while (pos >= 0) {
        fseek(calendar, pos, SEEK_SET);

        int c = fgetc(calendar);

        if (c == '\n') {
            pos++;
            break;
        }

        pos--;
    }
    
    if (pos < 0) {
        pos = 0;
    }

    fseek(calendar, pos, SEEK_SET);

    // Read line
    size_t capacity = 11;
    char* line = malloc(capacity + 1);
    memset(line, '\0', capacity + 1);

    if (!line) {
        fclose(calendar);
        return last_date;
    }

    if (!fgets(line, capacity, calendar)) {
        free(line);
        fclose(calendar);
        return last_date;
    }

    char year[5] = "\0";
    char month[3] = "\0";
    char day[3] = "\0";

    strncpy(year, line, 4);
    strncpy(month, line + 5, 2);
    strncpy(day, line + 8, 2);

    free(line);

    last_date.tm_year = atoi(year) - 1900;
    last_date.tm_mon = atoi(month) - 1;
    last_date.tm_mday = atoi(day);
    mktime(&last_date);

    fclose(calendar);
    return last_date;
}

/*
 * Expands the recurring event into an array of events.
 * On failure, the events array will have length 0.
 * */
struct events expand_rrule(struct event event) {
    struct events events = {0};
    init_events(&events);

    // Generate a preliminary array of dates using just FREQ, INTERVAL, and COUNT/UNTIL
    time_t until_time = event.rrule.until.tm_year != 0 ? 
        mktime(&event.rrule.until) : 0;

    struct event cur_event = event;
    time_t cur_time = mktime(&event.datetime);
    while (true) {
        struct event next_event = cur_event;
        time_t next_time;
        switch (event.rrule.freq) {
            case YEARLY: 
                next_event.datetime.tm_year += event.rrule.interval;
                break;
            case MONTHLY:
                next_event.datetime.tm_mon += event.rrule.interval;
                break;
            case WEEKLY:
                next_event.datetime.tm_mday += 7 * event.rrule.interval;
                break;
            case DAILY:
                next_event.datetime.tm_mday += event.rrule.interval;
                break;
            case HOURLY:
                next_event.datetime.tm_hour += event.rrule.interval;
                break;
            case MINUTELY:
                next_event.datetime.tm_min += event.rrule.interval;
                break;
            case NONE:
                return events;
        }
        next_time = mktime(&next_event.datetime);
        
        struct rrule blank_rrule = {0};
        cur_event.rrule = blank_rrule;

        if (event.rrule.count != 0 && events.length >= event.rrule.count) {
            break;
        } else if (until_time != 0 && next_time > until_time) {
            append_event(&events, cur_event);
            break;
        }

        append_event(&events, cur_event);

        cur_event = next_event;
        cur_time = next_time;
    }
         

    // Process each BYxxx rule


    return events;
}
