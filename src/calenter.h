#ifndef CALENTERM_H
#define CALENTERM_H

#include <stddef.h>
#include <ncurses.h>
#include "drivers/calendartxt.h"

#define DEBUG
#define ACTIVE_COLOR_PAIR 1
#define INACTIVE_COLOR_PAIR 2
#define INPUT_FIELD_PAIR 3

#define SCHEDULE_WIN 0
#define CALENDAR_WIN 1
#define CONTROLS_WIN 2

#define NUM_WINDOWS 3


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
 * Formats the given date as a human readable string in the following format:
 * "weekday, dd month yyyy" (e.g., Friday, 20 February 2026)
 */
void format_pretty_date(char* buffer, int year, int month, int day);

/*
 * Function to write output to a logfile instead of the terminal
 */
void debug_log(const char* format, ...);

void add_widget(Window* window, Widget widget);
int get_widget_index(Window* window, enum _widget_tag tag);

Window* create_win(int id, char* title, int height, int width, int startx, int starty);
void free_win(Window* window);
void refresh_win(Window* window, bool active);
void set_active_window(Window** active_win, Window* window);

void init_schedule(Widget* schedule);
void render_schedule(Window* win, bool active);

void init_calendar(Widget* calendar);
void render_calendar(Window* win, bool active);

char* get_month_name(int month);
int get_days_in_month(int month);
struct tm get_day_info(int year, int month, int day);

struct event add_event_modal(Window** windows, struct event* event);

#endif
