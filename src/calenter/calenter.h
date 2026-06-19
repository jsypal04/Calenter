#ifndef CALENTERM_H
#define CALENTERM_H

#include <stddef.h>
#include <ncurses.h>
#include "../common/calendartxt.h"

#define SCHEDULE_WIN 0
#define CALENDAR_WIN 1
#define CONTROLS_WIN 2
#define MODAL_WIN    50

#define NUM_WINDOWS 3
#define NUM_FOCUSABLE_WINDOWS 2

#define CTRL(x) ((x) & 0x1f)

typedef struct _calender_widget {
    int selected_day;
    int month;
    int year;
} Calendar;

typedef struct _schedule_widget {
    int day;
    int month;
    int year;
    int selected_event;
    struct events events;
} Schedule;

enum _widget_tag {
    CALENDAR = 100,
    SCHEDULE = 101,
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
    int       id;
    WINDOW*   win;
    char*     title;
    int       height;
    int       width;
    int       num_widgets;
    Widget*   widgets;
    UILayout* layout;
} Window;

/*
 * Formats the given date as a human readable string in the following format:
 * "weekday, dd month yyyy" (e.g., Friday, 20 February 2026)
 */
void format_pretty_date(char* buffer, int year, int month, int day);

void add_widget(Window* window, Widget widget);
int get_widget_index(Window* window, enum _widget_tag tag);

/*
 * Creates a window. Pass a NULL title for no title
 * */
Window* create_win(
    int id, char* title, int height, int width, int startx, int starty
);
void free_win(Window* window);
void refresh_win(Window* window, bool active);
void resize_win(Window* win, UILayout* layout);
void set_active_window(Window** active_win, Window* window);
void refresh_controls(int win_id);

void init_schedule(Widget* schedule);
void render_schedule(Window* win, bool active);

void init_calendar(Widget* calendar);
void render_calendar(Window* win, bool active);

/*
 * Sets the given widget (either Schedule or Calendar) to the specified date.
 * Returns -1 if the provided widget has a tag other than SCHEDULE or CALENDAR.
 * */
int move_widget_date(Widget* widget, int year, int month, int day);

char* get_month_name(int month);
int get_days_in_month(int month);
struct tm get_day_info(int year, int month, int day);

struct event add_event_modal(Window** windows, struct event* event);

/*
 * Removes leading and trailing whitespace
 * */
void trim(char* str);

bool verify_date(char* date);

char* stringify_datetime(struct tm dt, size_t* len);

#endif
