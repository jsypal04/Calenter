#include <asm-generic/errno-base.h>
#include <assert.h>
#include <fcntl.h>
#include <libnotify/notification.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libnotify/notify.h>
#include <sys/stat.h>
#include <unistd.h>

#include "calenter.h"
#include "utils/config.h"
#include "utils/daemon-utils.h"
#include "utils/sync.h"
#include "utils/ncurses-utils.h"
#include "layout/layout.h"
#include "utils/debug.h"
#include "widgets/pane.h"



void handle_key_press(Window** active_win, int key);

Window* windows[NUM_WINDOWS];

int main() {
    debug_log("Starting UI...\n");
    notify_init("Calenter");

    Config config = read_config();

    handle_daemon_fifo(config);

    Window* active_win = NULL;
    int active_win_index = 0;
    int ch;

    setup_ncurses();

    UILayout* layout = new_layout(LINES, COLS, ROW);

    new_ui_pane(layout, SCHEDULE_WIN, "Daily Schedule");
    new_ui_pane(layout, CALENDAR_WIN, "Calendar");
    new_ui_pane(layout, CONTROLS_WIN, NULL);

    // Focusable windows
    windows[SCHEDULE_WIN] = create_win(
        SCHEDULE_WIN, "Daily Schedule",
        get_height(layout, SCHEDULE_WIN), get_width(layout, SCHEDULE_WIN),
        get_startx(layout, SCHEDULE_WIN), get_starty(layout, SCHEDULE_WIN)
    );
    windows[CALENDAR_WIN] = create_win(
        CALENDAR_WIN, "Calendar",
        get_height(layout, CALENDAR_WIN), get_width(layout, CALENDAR_WIN),
        get_startx(layout, CALENDAR_WIN), get_starty(layout, CALENDAR_WIN)
    );

    // Non-focusable windows
    windows[CONTROLS_WIN] = create_win(
        CONTROLS_WIN, NULL,
        get_width(layout, CONTROLS_WIN), get_width(layout, CONTROLS_WIN),
        get_startx(layout, CONTROLS_WIN), get_starty(layout, CONTROLS_WIN)
    );

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
                active_win_index =
                    active_win_index < NUM_FOCUSABLE_WINDOWS - 1 ?
                    active_win_index + 1 : 0;
                set_active_window(&active_win, windows[active_win_index]);
                break;
            }
            case KEY_BTAB: {
                active_win_index =
                    active_win_index > 0 ?
                    active_win_index - 1 :
                    NUM_FOCUSABLE_WINDOWS - 1;
                set_active_window(&active_win, windows[active_win_index]);
                break;
            }

            case 's':
            if (config.remote_url != NULL)
                sync_calendar(config.remote_url);
            break;

            case KEY_RESIZE:
            erase();
            refresh();
            resize_layout(layout, LINES, COLS);

            werase(windows[CONTROLS_WIN]->win);
            resize_win(windows[CONTROLS_WIN], layout);

            werase(windows[SCHEDULE_WIN]->win);
            resize_win(windows[SCHEDULE_WIN], layout);
            render_schedule(windows[SCHEDULE_WIN], active_win_index == SCHEDULE_WIN);
            refresh_win(windows[SCHEDULE_WIN], active_win_index == SCHEDULE_WIN);

            werase(windows[CALENDAR_WIN]->win);
            resize_win(windows[CALENDAR_WIN], layout);
            render_calendar(windows[CALENDAR_WIN], active_win_index == CALENDAR_WIN);
            refresh_win(windows[CALENDAR_WIN], active_win_index == CALENDAR_WIN);
            break;

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

    free_layout(layout);

    free(config.remote_url);
    config.remote_url = NULL;

    notify_uninit();
    return 0;
}

void handle_key_press(Window** active_win_ref, int key) {
    Window* active_win = *active_win_ref;

    if (active_win->id == CALENDAR_WIN) {
        switch (key) {
            case KEY_RIGHT:
            case 'l': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, 0, 1);
                werase(active_win->win);
                render_calendar(active_win, true);
                break;
            }
            case 'L': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, 1, 0);
                werase(active_win->win);
                render_calendar(active_win, true);
                break;
            }
            case 'H': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, -1, 0);
                werase(active_win->win);
                render_calendar(active_win, true);
                break;
            }
            case KEY_LEFT:
            case 'h': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, 0, -1);
                werase(active_win->win);
                render_calendar(active_win, true);
                break;
            }
            case KEY_UP:
            case 'k': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, 0, -7);
                werase(active_win->win);
                render_calendar(active_win, true);
                break;
            }
            case KEY_DOWN:
            case 'j': {
                int cal_index = get_widget_index(active_win, CALENDAR);
                move_widget_date(&active_win->widgets[cal_index], 0, 0, 7);
                werase(active_win->win);
                render_calendar(active_win, true);
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
            case KEY_RIGHT:
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
            case KEY_LEFT:
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
            case KEY_DOWN:
            case 'j': {
                int sched_index = get_widget_index(active_win, SCHEDULE);
                int num_events = active_win->widgets[sched_index].widget.schedule.events.length;
                if (active_win->widgets[sched_index].widget.schedule.selected_event < num_events) {
                    active_win->widgets[sched_index].widget.schedule.selected_event++;
                    render_schedule(active_win, true);
                }
                break;
            }
            case KEY_UP:
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

                if (new_event.summary == NULL) break;

                new_event.datetime.tm_year = active_win->widgets[sched_index].widget.schedule.year - 1900;
                new_event.datetime.tm_mon = active_win->widgets[sched_index].widget.schedule.month - 1;
                new_event.datetime.tm_mday = active_win->widgets[sched_index].widget.schedule.day;
                new_event.datetime.tm_isdst = -1;
                mktime(&new_event.datetime);

                if (new_event.rrule.freq == NONE) {
                    add_event(
                        new_event,
                        new_event.datetime.tm_year + 1900,
                        new_event.datetime.tm_mon + 1,
                        new_event.datetime.tm_mday
                    );
                } else {
                    struct events new_events_list = expand_rrule(new_event);
                    for (int i = 0; i < new_events_list.length; i++) {
                        struct event evt = new_events_list.events[i];
                        add_event(
                            evt,
                            evt.datetime.tm_year + 1900,
                            evt.datetime.tm_mon + 1,
                            evt.datetime.tm_mday
                        );
                    }
                }

                free_events(active_win->widgets[sched_index].widget.schedule.events);
                active_win->widgets[sched_index].widget.schedule.events =
                    get_events(
                            new_event.datetime.tm_year + 1900,
                            new_event.datetime.tm_mon + 1,
                            new_event.datetime.tm_mday
                    );

                render_schedule(active_win, true);
                break;
            }
        }
    }
}
