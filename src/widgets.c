#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "calenter.h"


void add_widget(Window* window, Widget widget) {
    if (window->widgets == NULL) {
        window->widgets = malloc(sizeof(Widget));
        window->widgets[0] = widget;
        window->num_widgets++;
        return;
    }

    Widget* new_widgets = malloc(window->num_widgets + 1);
    for (int i = 0; i < window->num_widgets; i++) {
        new_widgets[i] = window->widgets[i];
    }
    free(window->widgets);
    window->widgets = new_widgets;
    window->widgets++;
}

// Gets the index of the widget in the widget array. Returns -1 if not found.
int get_widget_index(Window* window, enum _widget_tag tag) {
    for (int i = 0; i < window->num_widgets; i++) {
        if (window->widgets[i].tag == tag) {
            return i;
        }
    }
    debug_log("Widget %d not found.\n", tag);
    exit(1);
}

void init_schedule(Widget* schedule) {
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    Schedule sched;
    sched.day = info->tm_mday;
    sched.month = info->tm_mon + 1;
    sched.year = info->tm_year + 1900;
    sched.selected_event = 0;

    sched.events = get_events(info->tm_year + 1900, info->tm_mon + 1, info->tm_mday);

    schedule->tag = SCHEDULE;
    schedule->widget.schedule = sched;
}

void init_calendar(Widget* calendar) {
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    Calendar cal;
    cal.selected_day = info->tm_mday;
    cal.month = info->tm_mon + 1;

    calendar->tag = CALENDAR;
    calendar->widget.calendar = cal;
}


void render_schedule(Window* win, bool active) {
    werase(win->win);

    int sched_index = get_widget_index(win, SCHEDULE);
    Schedule schedule = win->widgets[sched_index].widget.schedule;

    char header[100] = "\0";
    format_pretty_date(header, schedule.year, schedule.month, schedule.day);
    int header_length = strlen(header);

    mvwprintw(win->win, 1, (win->width - header_length) / 2, "%s", header);

    for (int i = 0; i < schedule.events.length; i++) {
        struct event event = schedule.events.events[i];
        char time_str[10];
        format_time(time_str, event.hour, event.min);

        if (i == schedule.selected_event) {
            wattron(win->win, A_REVERSE);
            mvwprintw(win->win, 3 + i * 2, 3, "%s - %s", time_str, event.summary);
            wattroff(win->win, A_REVERSE);
        } else {
            mvwprintw(win->win, 3 + i * 2, 3, "%s - %s", time_str, event.summary);
        }
    }

    if (schedule.events.length == schedule.selected_event) {
        wattron(win->win, A_REVERSE);
        mvwprintw(win->win, 3 + schedule.events.length * 2, 3, "Add event");
        wattroff(win->win, A_REVERSE);
    } else {
        mvwprintw(win->win, 3 + schedule.events.length * 2, 3, "Add event");
    }

    refresh_win(win, active);
}

void render_calendar(Window* win, bool active) {
    int cal_index = get_widget_index(win, CALENDAR);
    Calendar calendar = win->widgets[cal_index].widget.calendar;

    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    char* month_name = get_month_name(calendar.month);

    int header_length = strlen(month_name) + 5; // To account for the year

    mvwprintw(win->win, 2, (win->width - header_length) / 2, "%s %d", month_name, info->tm_year + 1900);

    char* wday_labels[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int i = 0; i < 7; i++) {
        wattron(win->win, A_UNDERLINE);
        mvwprintw(win->win, 4, (win->width - 21) / 2 + (i * 3), "%s", wday_labels[i]);
        wattroff(win->win, A_UNDERLINE);
    }

    int days_in_month = get_days_in_month(info->tm_mon + 1);
    int week_number = 1;
    for (int day = 1; day <= days_in_month; day++) {
        struct tm my_time = get_day_info(info->tm_year + 1900, info->tm_mon + 1, day);

        assert(day <= 31);

        char day_str[3];
        if (day < 10) {
            sprintf(day_str, "0%d", day);
        } else {
            sprintf(day_str, "%d", day);
        }

        if (calendar.selected_day == day) {
            wattron(win->win, A_REVERSE);
            mvwprintw(win->win, week_number + 5, (win->width - 21) / 2 + my_time.tm_wday * 3, "%s", day_str);
            wattroff(win->win, A_REVERSE);
        } else {
            mvwprintw(win->win, week_number + 5, (win->width - 21) / 2 + my_time.tm_wday * 3, "%s", day_str);
        }

        if (my_time.tm_wday == 6) {
            week_number++;
        }
    }

    refresh_win(win, active);
}
