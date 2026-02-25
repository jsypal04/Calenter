#include <assert.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
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

void handle_key_press(Window** active_win, int key);

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
