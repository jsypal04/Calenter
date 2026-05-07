/*
 * ics.c
 *
 * This file contains functions to parse events in a ICS
 * file into data structures that can be written to calendar.txt
 * using the functions in calendartxt.c.
 */

#include "calendartxt.h"
#include "../utils/array.h"
#include <assert.h>
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
 *
 * TODO: Make tests for this function
 * */
int parse_ISO_8601_timestamp(char* timestamp, struct tm* time) {
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
    time->tm_year = atoi(year) - 1900;
    time->tm_mon = atoi(month) - 1;
    time->tm_mday = atoi(day);

    if (strlen(timestamp) == 8) {
        mktime(time); 
        return 0;
    }

    if (timestamp[8] != 'T' || strlen(timestamp) < 15)
        return -1;

    strncpy(hour, timestamp + 9, 2);
    strncpy(min, timestamp + 11, 2);

    // TODO: Validate the atoi inputs
    time->tm_hour = atoi(hour);
    time->tm_min = atoi(min);
    mktime(time);

    // Handle timezone conversion
    if (strlen(timestamp) == 16 && timestamp[15] == 'Z') {
        time_t t = timegm(time);
        localtime_r(&t, time);
    }

    return 0;
}

enum FREQ handle_freq(char* value) {
    if (strcmp(value, "MINUTELY") == 0) {
        return MINUTELY;
    } else if (strcmp(value, "HOURLY") == 0) {
        return HOURLY;
    } else if (strcmp(value, "DAILY") == 0) {
        return DAILY;
    } else if (strcmp(value, "WEEKLY") == 0) {
        return WEEKLY;
    } else if (strcmp(value, "MONTHLY") == 0) {
        return MONTHLY;
    } else if (strcmp(value, "YEARLY") == 0) {
        return YEARLY;
    } else {
        return NONE;
    }
}

Array* handle_by_numeric_time_unit(char* value) {
    Array* array = new_array(10, INT);

    int start = 0;
    for (int i = 0; i < strlen(value) - 1; i++) {
        if (value[i] != ',') continue;
        
        assert(i - start <= 4);

        char tmp[5] = "\0";
        strncpy(tmp, value + start, i - start);

        append_int(array, atoi(tmp)); // Maybe valitate input to atoi??
        start = i + 1;
    }

    assert(strlen(value) - start <= 4);

    char tmp[5] = "\0";
    strncpy(tmp, value + start, strlen(value) - start);
    append_int(array, atoi(tmp));

    return array;
}

Array* handle_by_categorical_time_unit(char* value) {
    Array* array = new_array(10, STRING);

    int start = 0;
    for (int i = 0; i < strlen(value); i++) {
        if (value[i] != ',') continue;

        char* tmp = strndup(value + start, i - start);
        append_string(array, tmp);
        free(tmp);
        tmp = NULL;
        start = i + 1;
    }

    char* tmp = strndup(value + start, strlen(value) - start);
    append_string(array, tmp);
    free(tmp);
    tmp = NULL;

    return array;
}

enum WKDAY handle_wkst(char* value) {
    if (strcmp(value, "SU") == 0) {
        return SU;
    } else if (strcmp(value, "MO") == 0) {
        return MO;
    } else if (strcmp(value, "TU") == 0) {
        return TU;
    } else if (strcmp(value, "WE") == 0) {
        return WE;
    } else if (strcmp(value, "TH") == 0) {
        return TH;
    } else if (strcmp(value, "FR") == 0) {
        return FR;
    } else if (strcmp(value, "SA") == 0) {
        return SA;
    } else {
        return -1;
    }
}

/*
 * NOTE: Recurrence Rules with FREQ=SECONDLY will be ignored since
 * we only store up to minutes in calendar.txt.
 * */
struct rrule parse_rrule(char* raw_rrule) {
    struct rrule rrule = {0};

    int start = 0;
    int index = 0;
    char ch = raw_rrule[0];

    while (ch != '\0') {
        while (ch != '=') {
            index++;
            ch = raw_rrule[index];
        }

        char name[32] = "\0";
        strncpy(name, raw_rrule + start, index - start);
        // printf("\tname = %s\n", name);
        
        index++;
        start = index;
        while (ch != ';' && ch != '\0') {
            index++;
            ch = raw_rrule[index];
        }

        char* value = malloc(sizeof(char) * 2 * (index - start));
        memset(value, '\0', sizeof(char) * 2 * (index - start));
        strncpy(value, raw_rrule + start, index - start);
        // printf("\tvalue = %s\n", value);

        if (strcmp(name, "FREQ") == 0) {
            rrule.freq = handle_freq(value);
        } else if (strcmp(name, "UNTIL") == 0) {
            struct tm timestamp = {0};
            parse_ISO_8601_timestamp(value, &timestamp);
            rrule.until = timestamp;
        } else if (strcmp(name, "COUNT") == 0) {
            rrule.count = atoi(value);
        } else if (strcmp(name, "INTERVAL") == 0) {
            rrule.interval = atoi(value);
        } else if (strcmp(name, "BYSECOND") == 0) {
            rrule.bysecond = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYMINTUTE") == 0) {
            rrule.byminute = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYHOUR") == 0) {
            rrule.byhour = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYDAY") == 0) {
            rrule.byday = handle_by_categorical_time_unit(value);
        } else if (strcmp(name, "BYMONTHDAY") == 0) {
            rrule.bymonthday = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYYEARDAY") == 0) {
            rrule.byyearday = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYWEEKNO") == 0) {
            rrule.byweekno = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYMONTH") == 0) {
            rrule.bymonth = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "BYSETPOS") == 0) {
            rrule.bysetpos = handle_by_numeric_time_unit(value);
        } else if (strcmp(name, "WKST") == 0) {
            rrule.wkst = handle_wkst(value);
        }

        if (rrule.interval == 0) rrule.interval = 1;

        free(value);

        index++;
        start = index;
    }

    return rrule;
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

    int eof_marker = 0;

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
                struct tm timestamp = {0};
                parse_ISO_8601_timestamp(cline.value, &timestamp);
                event.datetime = timestamp;
                if (strlen(cline.value) < 15) {
                    event.all_day = true;
                }
            } else if (strcmp(cline.name, "SUMMARY") == 0) {
                event.summary = strdup(cline.value);
            } else if (strcmp(cline.name, "RRULE") == 0) {
                // printf("RRULE = %s\n", cline.value);
                event.rrule = parse_rrule(cline.value);
            }
        }
        if (!event_skipped) {
            append_event(&events, event);
        }
    }

    fclose(line.ics_file);
    return events;
}

// int main(int argc, char *argv[]) {
//   if (argc != 2) {
//     printf("Usage: ./ics path/to/ics/file\n");
//     return 0;
//   }
//
//   parse_ics(argv[1]);
//
//   return 0;
// }
