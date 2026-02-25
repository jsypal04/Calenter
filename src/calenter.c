#include <assert.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "calenter.h"


void debug_log(const char* format, ...) {
#ifdef DEBUG
    #include <stdio.h>
    #define DEBUG_LOG_FILE "logs/debug.log"

    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);
    char buffer[80];

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);

    va_list args;
    va_start(args, format);

    FILE* debug_log_file = fopen(DEBUG_LOG_FILE, "a");

    fprintf(debug_log_file, "[%s] ", buffer);
    vfprintf(debug_log_file, format, args);
    va_end(args);

    fclose(debug_log_file);
#endif
}


Window* create_win(int id, char* title, int height, int width, int startx, int starty);
void free_win(Window* window);
void refresh_win(Window* window, bool active);
void set_active_window(Window** active_win, Window* window);
void add_widget(Window* window, Widget widget);

void handle_key_press(Window** active_win, int key);

void init_schedule(Widget* schedule);
void render_schedule(Window* win, bool active);

void init_calendar(Widget* calendar);
void render_calendar(Window* win, bool active);

char* get_month_name(int month);
int get_days_in_month(int month);
struct tm get_day_info(int year, int month, int day);

Window* windows[NUM_WINDOWS];

int main() {
    debug_log("Starting UI...\n");

    Window* active_win = NULL;
    int active_win_index = 0;
    int ch;

    initscr();
    set_escdelay(25);
    curs_set(0);
    clear();
    noecho();
    cbreak();
    start_color();

    init_pair(ACTIVE_COLOR_PAIR, COLOR_CYAN, COLOR_BLACK);
    init_pair(INACTIVE_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(INPUT_FIELD_PAIR, COLOR_WHITE, 8);

    windows[SCHEDULE_WIN] = create_win(SCHEDULE_WIN, "Daily Schedule", LINES - 4, 2 * COLS / 3, 0, 0);
    windows[CALENDAR_WIN] = create_win(CALENDAR_WIN, "Calendar", LINES - 4, COLS / 3, 2 * COLS / 3, 0);
    windows[CONTROLS_WIN] = create_win(CONTROLS_WIN, "Commands", 4, COLS, 0, LINES - 4);

    Widget calendar_widget;
    init_calendar(&calendar_widget);

    Widget schedule_widget;
    init_schedule(&schedule_widget);

    add_widget(windows[SCHEDULE_WIN], schedule_widget);
    add_widget(windows[CALENDAR_WIN], calendar_widget);

    render_schedule(windows[SCHEDULE_WIN], true);
    render_calendar(windows[CALENDAR_WIN], false);

    set_active_window(&active_win, windows[active_win_index]);

    while (true) {
        ch = wgetch(active_win->win);

        switch (ch) {
        case '\t': {
            int windows_len = sizeof(windows) / sizeof(Window*);
            if (active_win_index == windows_len - 1) {
                active_win_index = 0;
            } else {
                active_win_index++;
            }
            set_active_window(&active_win, windows[active_win_index]);
            break;
        }
        case ERR:
            debug_log("Received %d from wgetch\n", ch);
            free_win(windows[0]);
            free_win(windows[1]);
            endwin();
            exit(1);
        default: handle_key_press(&active_win, ch);
        };

        if (ch == 'q') {
            break;
        }
    }

    free_win(windows[0]);
    free_win(windows[1]);
    endwin();

    return 0;
}

Window* create_win(int id, char* title, int height, int width, int startx, int starty) {
    Window* window = malloc(sizeof(Window));

    window->id = id;
    window->title = strdup(title);
    window->width = width;
    window->height = height;
    window->widgets = NULL;
    window->win = newwin(height, width, starty, startx);

    keypad(window->win, true);

    refresh_win(window, false);

    return window;
}

void free_win(Window* window) {
    delwin(window->win);
    free(window->title);
    free(window->widgets);
    free(window);

    window = NULL;
}

void refresh_win(Window* window, bool active) {
    if (active) {
        wattron(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
        wattroff(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    } else {
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
    }
    wrefresh(window->win);
}

void set_active_window(Window** active_win, Window* window) {
    Window* current_active_win = *active_win;
    if (current_active_win != NULL) {
        refresh_win(current_active_win, false);
    }

    *active_win = window;
    wattron(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    refresh_win(window, true);
}

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

void handle_key_press(Window** active_win_ref, int key) {
    Window* active_win = *active_win_ref;

    if (active_win->id == CALENDAR_WIN) {
        switch (key) {
            case 'l': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                int days_in_month = get_days_in_month(active_win->widgets[cal_index].widget.calendar.month);
                if (active_win->widgets[cal_index].widget.calendar.selected_day != days_in_month) {
                    active_win->widgets[cal_index].widget.calendar.selected_day++;
                    render_calendar(active_win, true);
                }
                break;
            }
            case 'h': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                if (active_win->widgets[cal_index].widget.calendar.selected_day != 1) {
                    active_win->widgets[cal_index].widget.calendar.selected_day--;
                    render_calendar(active_win, true);
                }
                break;
            }
            case 'k': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                if (active_win->widgets[cal_index].widget.calendar.selected_day - 7 >= 1) {
                    active_win->widgets[cal_index].widget.calendar.selected_day -= 7;
                    render_calendar(active_win, true);
                }
                break;
            }
            case 'j': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                int days_in_month = get_days_in_month(active_win->widgets[cal_index].widget.calendar.month);
                if (active_win->widgets[cal_index].widget.calendar.selected_day + 7 <= days_in_month) {
                    active_win->widgets[cal_index].widget.calendar.selected_day += 7;
                    render_calendar(active_win, true);
                }
                break;
            }
            case 10: {
                int cal_index = get_widget_index(active_win, CALENDAR);
                int sched_index = get_widget_index(windows[SCHEDULE_WIN], SCHEDULE);

                int month = active_win->widgets[cal_index].widget.calendar.month;
                int day = active_win->widgets[cal_index].widget.calendar.selected_day;

                windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule.month = month;
                windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule.day = day;

                int year = windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule.year;

                free_events(windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule.events);
                windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule.events =
                    get_events(year, month, day);

                render_schedule(windows[SCHEDULE_WIN], false);
            }
        }
    } else if (active_win->id == SCHEDULE_WIN) {
        switch (key) {
            case 'l': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                int days_in_month = get_days_in_month(active_win->widgets[sched_index].widget.schedule.month);
                if (active_win->widgets[sched_index].widget.schedule.day < days_in_month) {
                    active_win->widgets[sched_index].widget.schedule.selected_event = 0;
                    active_win->widgets[sched_index].widget.schedule.day++;

                    int year = active_win->widgets[sched_index].widget.schedule.year;
                    int month = active_win->widgets[sched_index].widget.schedule.month;
                    int day = active_win->widgets[sched_index].widget.schedule.day;

                    free_events(active_win->widgets[sched_index].widget.schedule.events);
                    active_win->widgets[sched_index].widget.schedule.events =
                        get_events(year, month, day);

                    render_schedule(active_win, true);
                }
                break;
            }
            case 'h': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                if (active_win->widgets[sched_index].widget.schedule.day > 1) {
                    active_win->widgets[sched_index].widget.schedule.selected_event = 0;
                    active_win->widgets[sched_index].widget.schedule.day--;

                    int year = active_win->widgets[sched_index].widget.schedule.year;
                    int month = active_win->widgets[sched_index].widget.schedule.month;
                    int day = active_win->widgets[sched_index].widget.schedule.day;

                    free_events(active_win->widgets[sched_index].widget.schedule.events);
                    active_win->widgets[sched_index].widget.schedule.events =
                        get_events(year, month, day);

                    render_schedule(active_win, true);
                }
                break;
            }
            case 'j': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                int num_events = active_win->widgets[sched_index].widget.schedule.events.length;
                if (active_win->widgets[sched_index].widget.schedule.selected_event < num_events) {
                    active_win->widgets[sched_index].widget.schedule.selected_event++;
                    render_schedule(active_win, true);
                }
                break;
            }
            case 'k': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                if (active_win->widgets[sched_index].widget.schedule.selected_event > 0) {
                    active_win->widgets[sched_index].widget.schedule.selected_event--;
                    render_schedule(active_win, true);
                }
                break;
            }
            case 'd': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                int length = active_win->widgets[sched_index].widget.schedule.events.length;
                int cur_selection = active_win->widgets[sched_index].widget.schedule.selected_event;

                int year = active_win->widgets[sched_index].widget.schedule.year;
                int month = active_win->widgets[sched_index].widget.schedule.month;
                int day = active_win->widgets[sched_index].widget.schedule.day;

                if (cur_selection == length) break;

                delete_event(active_win->widgets[sched_index].widget.schedule.events.events[cur_selection]);

                free_events(active_win->widgets[sched_index].widget.schedule.events);
                active_win->widgets[sched_index].widget.schedule.events =
                    get_events(year, month, day);

                render_schedule(active_win, true);
                break;
            }
            case 10: {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                int length = active_win->widgets[sched_index].widget.schedule.events.length;
                int cur_selection = active_win->widgets[sched_index].widget.schedule.selected_event;

                struct event new_event;
                if (cur_selection == length) {
                    new_event = add_event_modal(windows, NULL);
                } else {
                    new_event = add_event_modal(windows,
                        active_win->widgets[sched_index].widget.schedule.events.events + cur_selection);
                    if (new_event.summary != NULL) {
                        delete_event(active_win->widgets[sched_index].widget.schedule.events.events[cur_selection]);
                    }
                }

                if (new_event.summary != NULL) {
                    new_event.year = active_win->widgets[sched_index].widget.schedule.year;
                    new_event.month = active_win->widgets[sched_index].widget.schedule.month;
                    new_event.day = active_win->widgets[sched_index].widget.schedule.day;

                    add_event(new_event, new_event.year, new_event.month, new_event.day);

                    free_events(active_win->widgets[sched_index].widget.schedule.events);
                    active_win->widgets[sched_index].widget.schedule.events =
                        get_events(new_event.year, new_event.month, new_event.day);

                    render_schedule(active_win, true);
                }
                break;
            }
        }
    }
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

// TODO: Handle leap years.
int get_days_in_month(int month) {
    switch (month) {
        case 1: return 31;
        case 2: return 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return -1;
    };
}

char* get_month_name(int month) {
    switch (month) {
        case 1: return "January";
        case 2: return "February";
        case 3: return "March";
        case 4: return "April";
        case 5: return "May";
        case 6: return "June";
        case 7: return "July";
        case 8: return "August";
        case 9: return "September";
        case 10: return "October";
        case 11: return "November";
        case 12: return "December";
        default: return NULL;
    };
}

char* get_wday_name(int wday) {
    switch (wday) {
        case 0: return "Sunday";
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        default: return NULL;
    }
}

void format_pretty_date(char *buffer, int year, int month, int day) {
    struct tm day_info = get_day_info(year, month, day);

    char* month_name = get_month_name(month);
    char* wday_name = get_wday_name(day_info.tm_wday);

    sprintf(buffer, "%s, %d %s %d", wday_name, day, month_name, year);
}

struct tm get_day_info(int year, int month, int day) {
    struct tm my_time;
    memset(&my_time, 0, sizeof(struct tm));

    my_time.tm_year = year - 1900;
    my_time.tm_mon = month - 1;
    my_time.tm_mday = day;
    my_time.tm_isdst = -1;

    mktime(&my_time);

    return my_time;
}
