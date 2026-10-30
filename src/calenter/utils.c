#include <libnotify/notification.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "calenter.h"
#include "array.h"
#include "calendartxt.h"
#include "json.h"

// TODO: Handle leap years.
int get_days_in_month(int month) {
    switch (month) {
        case  1: return 31;
        case  2: return 28;
        case  3: return 31;
        case  4: return 30;
        case  5: return 31;
        case  6: return 30;
        case  7: return 31;
        case  8: return 31;
        case  9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return -1;
    };
}

char* get_month_name(int month) {
    switch (month) {
        case  1: return "January";
        case  2: return "February";
        case  3: return "March";
        case  4: return "April";
        case  5: return "May";
        case  6: return "June";
        case  7: return "July";
        case  8: return "August";
        case  9: return "September";
        case 10: return "October";
        case 11: return "November";
        case 12: return "December";
        default: return NULL;
    };
}

char* get_wday_name(int wday) {
    switch (wday) {
        case  0: return "Sunday";
        case  1: return "Monday";
        case  2: return "Tuesday";
        case  3: return "Wednesday";
        case  4: return "Thursday";
        case  5: return "Friday";
        case  6: return "Saturday";
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

bool verify_date(char* date) {
    bool valid = true;
    for (int i = 0; i < strlen(date); i++) {
        char c = date[i];
        if (
            (i != 4 && i != 7 && (c < 48 || c > 57)) ||
            ((i == 4 || i == 7) && c != '-')
        ) {
            valid = false;
            break;
        }
    }
    return valid;
}

// This is primarily used for debugging
char* stringify_datetime(struct tm dt, size_t* len) {
    *len = 1024;
    char* str = malloc(sizeof(char) * (*len));
    bzero(str, sizeof(char) * (*len));

    sprintf(str,
        "{'tm_year': %d, 'tm_mon': %d, 'tm_mday': %d, 'tm_yday': %d,"
        "'tm_wday': %d, 'tm_hour': %d, 'tm_min': %d, 'tm_sec': %d,"
        "'tm_zone': %s, 'tm_isdst': %d, 'tm_gmtoff': %ld}",
        dt.tm_year, dt.tm_mon, dt.tm_mday, dt.tm_yday, dt.tm_wday, dt.tm_hour,
        dt.tm_min, dt.tm_sec, dt.tm_zone, dt.tm_isdst, dt.tm_gmtoff
    );

    return str;
}

/**
 * Splits a single line into an array of lines depending on
 * the container width
 * */
Array* split_line(char* line, int container_width) {
    const int line_length = strlen(line);
    Array* lines = new_array(5, STRING);

    if (line_length < container_width) {
        append_string(lines, line);
        return lines;
    }

    while (strlen(line) >= container_width) {
        trim(line);
        for (int i = container_width - 1; i >= 0; i--) {
            if (i == 0) {
                char* new_line = strndup(line, container_width);
                trim(new_line);
                append_string(lines, new_line);
                free(new_line);
                new_line = NULL;
                line += container_width;
                break;
            }

            if (line[i] != ' ') continue;

            char* new_line = strndup(line, i + 1);
            trim(new_line);
            append_string(lines, new_line);
            free(new_line);
            new_line = NULL;
            line += i;
            break;
        }
    }
    trim(line);
    append_string(lines, line);

    return lines;
}

char* dump_ui_state(Window** windows, int active_window_id) {
    int sched_index = get_widget_index(windows[SCHEDULE_WIN], SCHEDULE);
    int cal_index   = get_widget_index(windows[CALENDAR_WIN], CALENDAR);

    Schedule schedule_widget = windows[SCHEDULE_WIN]->widgets[sched_index].widget.schedule;
    Calendar calendar_widget = windows[CALENDAR_WIN]->widgets[cal_index].widget.calendar;

    bool sched_active = false;
    bool cal_active = false;

    switch (active_window_id) {
        case SCHEDULE_WIN:
            sched_active = true;
            break;
        case CALENDAR_WIN:
            cal_active = true;
            break;
    }

    char* schedule_json = dump_schedule_state(schedule_widget, sched_active);
    char* calendar_json = dump_calendar_state(calendar_widget, cal_active);

    size_t buff_len = strlen(schedule_json) + strlen(calendar_json) + 200;
    char* json_buffer = malloc(sizeof(char) * buff_len);
    bzero(json_buffer, buff_len);

    sprintf(
        json_buffer,
        "{\"state\": {%s, %s}}",
        schedule_json,
        calendar_json
    );
    free(schedule_json);
    free(calendar_json);

    schedule_json = NULL;
    calendar_json = NULL;

    return json_buffer;
}

void send_notification(const char* msg) {
    NotifyNotification* noti = notify_notification_new(
        "Calenter",
        msg,
        "" // TODO: probably want to add icon here
    );
    notify_notification_show(noti, NULL);
    g_object_unref(G_OBJECT(noti));
}
