#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
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
#include "utils/sync.h"

#define DAEMON_FIFO "/tmp/calenter-notification-daemon.fifo"

void debug_log(const char* format, ...) {
#ifdef DEBUG
    #include <time.h>
    #include <stdio.h>
    #define DEBUG_LOG_FILE "/.calendar/logs/debug.log"

    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);
    char buffer[80];

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);

    char log_path[1024] = "\0";
    char* home = getenv("HOME");
    if (home == NULL) return;

    sprintf(log_path, "%s%s", home, DEBUG_LOG_FILE);

    va_list args;
    va_start(args, format);

    FILE* debug_log_file = fopen(log_path, "a");

    fprintf(debug_log_file, "[%s] ", buffer);
    vfprintf(debug_log_file, format, args);
    va_end(args);

    fclose(debug_log_file);
#endif
}

void start_notification_daemon(Config config);
void handle_key_press(Window** active_win, int key);

Window* windows[NUM_WINDOWS];

int main() {
    debug_log("Starting UI...\n");
    notify_init("Calenter");

    Config config = read_config();
    if (config.enable_notifications) {
        start_notification_daemon(config);
    }
    else {
        int fifo_fd = open(DAEMON_FIFO, O_WRONLY);
        if (fifo_fd != -1) {
            char buf[2] = "k";
            write(fifo_fd, buf, 2);
            close(fifo_fd);
        }
    }

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

    init_color(DARK_GREY, 251, 251, 251);

    init_pair(ACTIVE_COLOR_PAIR, COLOR_GREEN, COLOR_BLACK);
    init_pair(INACTIVE_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(INPUT_FIELD_PAIR, COLOR_WHITE, DARK_GREY);
    init_pair(ACTIVE_INPUT_FIELD_PAIR, COLOR_WHITE, 8);
    init_pair(CONTROLS_COLOR_PAIR, COLOR_BLUE, COLOR_BLACK);

    // Focusable windows
    windows[SCHEDULE_WIN] = create_win(SCHEDULE_WIN, "Daily Schedule", LINES - 4, 2 * COLS / 3 - 1, 1, 0);
    windows[CALENDAR_WIN] = create_win(CALENDAR_WIN, "Calendar", LINES - 4, COLS / 3, 2 * COLS / 3, 0);

    // Non-focusable windows
    windows[CONTROLS_WIN] = create_win(CONTROLS_WIN, NULL, 4, COLS, 0, LINES - 4);

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

    free(config.remote_url);
    config.remote_url = NULL;

    notify_uninit();
    return 0;
}

void start_notification_daemon(Config config) {
    debug_log("Starting notification daemon...\n");
    FILE* fp = popen("pidof calenter-notification-daemon", "r");
    char pid_buff[16];

    if (fgets(pid_buff, sizeof(pid_buff), fp) != NULL) {
        debug_log("Daemon already running\n");
        pclose(fp);
        return;
    }

    char proc_dir[1024] = "\0";
    ssize_t len = readlink("/proc/self/exe", proc_dir, sizeof(proc_dir) - 1);
    if (len == -1) {
        debug_log("Failed to start notification daemon.\n");
        return;
    }

    if (mkfifo(DAEMON_FIFO, 0666) < 0) {
       if (errno != EEXIST) {
           debug_log("Failed to create FIFO. Daemon not started.\n");
           return;
       }
    }

    debug_log("proc_dir: %s\n", proc_dir);
    char noti_path[2048] = "\0";
    strncpy(noti_path, proc_dir, 1024);
    strcpy(noti_path + strlen(proc_dir), "-notification-daemon");
    debug_log("noti_path: %s\n", noti_path);

    int pid = fork();
    if (pid == 0) {
        if (setsid() < 0) exit(EXIT_FAILURE);
        umask(0);
        chdir("/");

        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        int retval = execl(noti_path, "calenter-notification-daemon", NULL);
        if (retval == -1) {
            NotifyNotification* noti = notify_notification_new(
                "Daemon Error",
                "Failed to start the notification daemon",
                ""
            );
            notify_notification_show(noti, NULL);
            g_object_unref(G_OBJECT(noti));
            notify_uninit();
            _exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        int fifo_fd = open(DAEMON_FIFO, O_WRONLY);
        char buf[10] = "\0";
        sprintf(buf, "t%d", config.notify_time);
        write(fifo_fd, buf, sizeof(buf));
        close(fifo_fd);

        debug_log("started noti daemon\n");
    }
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
