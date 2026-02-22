#ifndef CALENTERM_H
#define CALENTERM_H

#include <stddef.h>
#include <ncurses.h>

#define DEBUG
#define ACTIVE_COLOR_PAIR 1
#define INACTIVE_COLOR_PAIR 2
#define INPUT_FIELD_PAIR 3

#define SCHEDULE_WIN 0
#define CALENDAR_WIN 1
#define CONTROLS_WIN 2

#define NUM_WINDOWS 3

// for all day events, hour == min == -1
struct event {
  int year;
  int month;
  int day;
  int hour;
  int min;
  char* summary;
};

struct events {
  size_t size;
  size_t length;
  struct event* events;
};

typedef struct _calender_widget {
    int selected_day;
    int month;
} Calendar;

typedef struct _schedule_widget {
    int day;
    int month;
    int year;
    int selected_event;
    struct events events;
} Schedule;

enum _widget_tag {
    CALENDAR,
    SCHEDULE,
};

union _widget_data {
    Calendar calendar;
    Schedule schedule;
};

typedef struct _widget {
    enum _widget_tag tag;
    union _widget_data widget;
} Widget;

typedef struct _window {
    int id;
    WINDOW* win;
    char* title;
    int width;
    int height;
    int num_widgets;
    Widget* widgets;
} Window;

/*
 * Gets an array of all the events for a given day from calendar.txt
 */
struct events get_events(int year, int month, int day);

/*
 * Writes the event to calendar.txt
 */
void add_event(struct event event, int year, int month, int day);

/*
 * Inializes an empty events array with initial size of 10
 */
void init_events(struct events* events);

/*
 * Adds the event to the end of the events array
 */
void append_event(struct events* events, struct event new_event);

/*
 * Inserts event into events preserving chronological order
 */
void insert_event(struct events* events, struct event new_event);


/*
 * Frees the fields that are dynamically allocated in get events
 */
void free_events(struct events events);


/*
 * Returns -1 if time 1 is before time 2, 1 if time 1 is after time 2
 * and 0 if they are the same
 */
int time_cmp(int hour1, int min1, int hour2, int min2);

/*
 * Formats the time in 24-hour format like so: "HH:MM".
 * `hour = -1` indicates an all day event formatted like: "ALL DAY".
 *
 * Note: `buffer` should be at least 7 characters long.
 */
void format_time(char* buffer, int hour, int min);

/*
 * Formats the given date as a human readable string in the following format:
 * "weekday, dd month yyyy" (e.g., Friday, 20 February 2026)
 */
void format_pretty_date(char* buffer, int year, int month, int day);

/*
 * Formats the given date for calendar.txt: "yyyy-mm-dd"
 */
void format_calendartxt_date(char* buffer, int year, int month, int day);

/*
 * Function to write output to a logfile instead of the terminal
 */
void debug_log(const char* format, ...);


void refresh_win(Window* window, bool active);

struct event add_event_modal(Window** windows);

#endif
