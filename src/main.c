#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "calenterm.h"

#define DEBUG
#define ACTIVE_COLOR_PAIR 1
#define INACTIVE_COLOR_PAIR 2

#define SCHEDULE_WIN 0
#define CALENDAR_WIN 1
#define CONTROLS_WIN 2

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

typedef struct _calender_widget {
    int selected_day;
    int month;
} Calendar;

enum _widget_tag {
    CALENDAR
};

union _widget_data {
    Calendar calendar;
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

Window* create_win(int id, char* title, int height, int width, int startx, int starty);
void free_win(Window* window);
void refresh_win(Window* window, bool active);
void set_active_window(Window** active_win, Window* window);
void add_widget(Window* window, Widget widget);

void handle_key_press(Window** active_win, int key);

void init_calendar(Widget* calendar);
void render_calendar(Window* win, bool active);
char* get_month_name(int month, int* length);
int get_days_in_month(int month);

int main() {
    debug_log("Starting UI...\n");

    Window* windows[3];
    Window* active_win = NULL;
    int active_win_index = 0;
    int ch;

    initscr();
    curs_set(0);
    clear();
    noecho();
    cbreak();
    start_color();

    init_pair(ACTIVE_COLOR_PAIR, COLOR_CYAN, COLOR_BLACK);
    init_pair(INACTIVE_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);

    windows[SCHEDULE_WIN] = create_win(SCHEDULE_WIN, "Daily Schedule", LINES - 4, 2 * COLS / 3, 0, 0);
    windows[CALENDAR_WIN] = create_win(CALENDAR_WIN, "Calendar", LINES - 4, COLS / 3, 2 * COLS / 3, 0);
    windows[CONTROLS_WIN] = create_win(CONTROLS_WIN, "Commands", 4, COLS, 0, LINES - 4);

    Widget calendar_widget;
    init_calendar(&calendar_widget);

    add_widget(windows[CALENDAR_WIN], calendar_widget);
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
    return -1;
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
            case KEY_ENTER: {
                // TODO: implement goto Daily Schedule for selected_day
            }
        }
    }
}

void init_calendar(Widget* calendar) {
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    Calendar cal;
    cal.selected_day = info->tm_mday;
    cal.month = info->tm_mon;

    calendar->tag = CALENDAR;
    calendar->widget.calendar = cal;
}

void render_calendar(Window* win, bool active) {
    int cal_index = get_widget_index(win, CALENDAR);
    Calendar calendar = win->widgets[cal_index].widget.calendar;

    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    int header_length;
    char* month_name = get_month_name(calendar.month, &header_length);

    header_length += 5; // To account for the year

    mvwprintw(win->win, 2, (win->width - header_length) / 2, "%s %d", month_name, info->tm_year + 1900);

    char* wday_labels[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int i = 0; i < 7; i++) {
        wattron(win->win, A_UNDERLINE);
        mvwprintw(win->win, 4, (win->width - 21) / 2 + (i * 3), "%s", wday_labels[i]);
        wattroff(win->win, A_UNDERLINE);
    }

    int days_in_month = get_days_in_month(info->tm_mon);
    int week_number = 1;
    for (int day = 1; day <= days_in_month; day++) {
        struct tm my_time;
        memset(&my_time, 0, sizeof(struct tm));

        my_time.tm_year = info->tm_year;
        my_time.tm_mon = info->tm_mon;
        my_time.tm_mday = day;
        my_time.tm_isdst = -1;

        mktime(&my_time);

        int insertion_index = my_time.tm_wday * 3;
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
        case 0: return 31;
        case 1: return 28;
        case 2: return 31;
        case 3: return 30;
        case 4: return 31;
        case 5: return 30;
        case 6: return 31;
        case 7: return 31;
        case 8: return 30;
        case 9: return 31;
        case 10: return 30;
        case 11: return 31;
        default: return -1;
    };
}

char* get_month_name(int month, int* length) {
    switch (month) {
        case 0:
            *length = sizeof("January");
            return "January";
        case 1:
            *length = sizeof("February");
            return "February";
        case 2:
            *length = sizeof("March");
            return "March";
        case 3:
            *length = sizeof("April");
            return "April";
        case 4:
            *length = sizeof("May");
            return "May";
        case 5:
            *length = sizeof("June");
            return "June";
        case 6:
            *length = sizeof("July");
            return "July";
        case 7:
            *length = sizeof("August");
            return "August";
        case 8:
            *length = sizeof("September");
            return "September";
        case 9:
            *length = sizeof("October");
            return "October";
        case 10:
            *length = sizeof("November");
            return "November";
        case 11:
            *length = sizeof("December");
            return "December";
        default: return NULL;
    };
}
