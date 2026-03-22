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

#include "calendartxt.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INIT_EVENTS_SIZE 500

typedef struct _ics_line {
  char *line;
  int length;
  int current_index;
  FILE *ics_file;
} Line;

typedef struct _param {
  char *name;
  char **values;
  int num_values;
  int values_cap;
} Param;

typedef struct _content_line {
  char *name;
  Param *params;
  int num_params;
  int params_cap;
  char* value;
} ContentLine;

void free_content_line(ContentLine* cline) {
    free(cline->name);
    cline->name = NULL;

    for (int i = 0; i < cline->num_params; i++) {
        free(cline->params[i].name);
        cline->params[i].name = NULL;

        for (int j = 0; j < cline->params[i].num_values; j++) {
            free(cline->params[i].values[j]);
            cline->params[i].values[j] = NULL;
        }
        free(cline->params[i].values);
        cline->params[i].values = NULL;
    }
    free(cline->params);
    cline->params = NULL;

    free(cline->value);
    cline->value = NULL;
}

/*
 * Double the capacity of the given array
 * */
void expand_str_array(char **arr, int length) {
    char **new_arr = malloc(sizeof(char *) * 2 * length);
    for (int i = 0; i < length; i++) {
      new_arr[i] = strdup(arr[i]);
      free(arr[i]);
      arr[i] = NULL;
    }
    arr = new_arr;
}

/*
 * Gets the next line in the ics file (accounting for line folding).
 * Returns the number of characters read or EOF if the end of file was reached.
 * */
int get_line(Line *line) {
    int buf_len = 2048;
    char *buffer = malloc(sizeof(char) * buf_len);
    memset(buffer, '\0', buf_len);

    char c;
    int index = 0;

    // I know this is probably dumb to have while (true) loop but I don't care
    while (true) {
      while ((c = fgetc(line->ics_file)) != '\r') {
        if (c == EOF)
          return EOF;

        if (index == buf_len) {
          char *bigger_buffer = malloc(sizeof(char) * 2 * buf_len);
          memset(buffer, '\0', buf_len);

          for (int i = 0; i < buf_len; i++) {
            bigger_buffer[i] = buffer[i];
          }
          buf_len *= 2;

          free(buffer);
          buffer = bigger_buffer;
        }

        buffer[index] = c;
        index++;
      }

      c = fgetc(line->ics_file); // takes care of the newline
      c = fgetc(line->ics_file);
      if (c != ' ' && c != '\t') {
          break;
      }
    }
    ungetc(c, line->ics_file);

    line->line = buffer;
    line->length = buf_len;

    return index;
}

char *parse_name(Line *line) {
    assert(line->current_index == 0);

    char c = line->line[0];
    while (c != ';' && c != ':') {
        line->current_index++;
        c = line->line[line->current_index];
    }

    char *name = malloc(sizeof(char) * line->current_index + 10);
    memset(name, '\0', sizeof(char) * line->current_index + 10);

    strncpy(name, line->line, line->current_index);
    return name;
}

Param parse_param_list(Line *line) {
  assert(line->line[line->current_index] == ';');

  Param param = {0};
  char c;
  int start_index = line->current_index + 1;

  do {
    line->current_index++;
    c = line->line[line->current_index];
  } while (c != '=');

  param.name = malloc(sizeof(char) * (line->current_index - start_index) + 10);
  memset(param.name, '\0',
         sizeof(char) * (line->current_index - start_index) + 10);
  strncpy(param.name, line->line + start_index,
          line->current_index - start_index);

  param.values = malloc(sizeof(char *) * 50);

  start_index = line->current_index + 1;
  do {
    line->current_index++;
    c = line->line[line->current_index];
  } while (c != ',' && c != ':');

  char *value = malloc(sizeof(char) * (line->current_index - start_index) + 10);
  memset(value, '\0', sizeof(char) * (line->current_index - start_index) + 10);
  strncpy(value, line->line + start_index, line->current_index - start_index);

  return param;
}

ContentLine parse_content_line(Line line) {
    ContentLine cline = {0};

    // parse name
    cline.name = parse_name(&line);

    // parse the list of params
    if (line.line[line.current_index] == ';') {
      parse_param_list(&line);
    }

    // parse value
    cline.value = strdup(line.line + line.current_index + 1);

    return cline;
}

/*
 * Parses the string timestamp and populates the date and time fields
 * of the event.
 *
 * Returns 0 if the parsing was successful, otherwise -1.
 * */
int parse_ISO_8601_timestamp(char* timestamp, struct event* event) {
    if (strlen(timestamp) < 8) return -1;

    char  year[10] = "\0";
    char month[10] = "\0";
    char   day[10] = "\0";
    char  hour[10] = "\0";
    char   min[10] = "\0";

    strncpy(year, timestamp, 4);
    strncpy(month, timestamp + 4, 2);
    strncpy(day, timestamp + 6, 2);

    // TODO: Validate the atoi inputs
    event->year  = atoi(year);
    event->month = atoi(month);
    event->day   = atoi(day);

    if (strlen(timestamp) == 8) {
        event->hour = -1;
        event->min  = -1;
        return 0;
    }

    if (timestamp[8] != 'T' || strlen(timestamp) < 15)
        return -1;

    strncpy(hour, timestamp + 9, 2);
    strncpy(min, timestamp + 11, 2);

    // TODO: Validate the atoi inputs
    event->hour = atoi(hour);
    event->min  = atoi(min);

    // Handle timezone conversion
    if (strlen(timestamp) == 16 && timestamp[15] == 'Z') {
        struct tm utc_time = {0};
        utc_time.tm_year = event->year - 1900;
        utc_time.tm_mon  = event->month - 1;
        utc_time.tm_mday = event->day;
        utc_time.tm_hour = event->hour;
        utc_time.tm_min  = event->min;
        mktime(&utc_time);

        time_t t = timegm(&utc_time);

        struct tm local_time = {0};
        localtime_r(&t, &local_time);

        event->year  = local_time.tm_year + 1900;
        event->month = local_time.tm_mon + 1;
        event->day   = local_time.tm_mday;
        event->hour  = local_time.tm_hour;
        event->min   = local_time.tm_min;
    }

    return 0;
}

struct events parse_ics(char *path) {
    Line line = {0};
    line.ics_file = fopen(path, "r");

    struct events events = {0};
    init_events(&events);

    struct tm today = {0};
    time_t t = time(NULL);
    localtime_r(&t, &today);
    
    get_line(&line);
    ContentLine cline = parse_content_line(line);

    int eof_marker;

    while (eof_marker != EOF) {
        while (eof_marker != EOF && (strcmp(cline.name, "BEGIN") != 0 || strcmp(cline.value, "VEVENT") != 0)) {
            free_content_line(&cline);
            eof_marker = get_line(&line);
            cline = parse_content_line(line);
        }

        if (eof_marker == EOF) break;

        struct event event = {0};
        bool event_skipped = false;

        while (eof_marker != EOF && (strcmp(cline.name, "END") != 0 || strcmp(cline.value, "VEVENT") != 0)) {
            free_content_line(&cline);
            eof_marker = get_line(&line);
            cline = parse_content_line(line);

            if (strcmp(cline.name, "DTSTART") == 0) {
                parse_ISO_8601_timestamp(cline.value, &event);
                printf("DTSTART = %s\n", cline.value);
            } else if (strcmp(cline.name, "SUMMARY") == 0) {
                printf("SUMMARY = %s\n", cline.value);
                event.summary = strdup(cline.value);
            }
        }
        if (!event_skipped) {
            append_event(&events, event);
            printf("-------------\n");
        }
    }

    fclose(line.ics_file);
    return events;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: ./ics path/to/ics/file\n");
  }

  parse_ics(argv[1]);

  return 0;
}
