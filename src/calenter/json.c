/**
 * json.c
 *
 * This file stringifies the UI state and broadcasts it over a unix domain
 * socket. It has no functionality to parse json
 * */

#include <semaphore.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

#include "calendartxt.h"
#include "calenter.h"
#include "debug.h"
#include "channel.h"


Channel_C* broadcast_channel;

/**
 * Doubles the size of the buffer passed
 * */
void expand_buffer(char* buffer, size_t buffer_len, size_t buffer_cap) {
    buffer_cap *= 2;
    char* new_buffer = malloc(sizeof(char) * buffer_cap);
    bzero(new_buffer, sizeof(char) * buffer_cap);
    strncpy(new_buffer, buffer, buffer_cap);
    free(buffer);
    buffer = new_buffer;
}


char* dump_events(struct events events) {
    int buffer_cap = 2048;
    int buffer_len = 0;
    char* buffer = malloc(sizeof(char) * buffer_cap);
    bzero(buffer, sizeof(char) * buffer_cap);

    buffer[buffer_len] = '[';
    buffer_len++;

    for (int i = 0; i < events.length; i++) {
        if (buffer_len == buffer_cap) {
            expand_buffer(buffer, buffer_len, buffer_cap);
        }

        struct event event = events.events[i];
        
        buffer[buffer_len] = '"';
        buffer_len++;

        format_time(buffer + buffer_len, event.datetime.tm_hour, event.datetime.tm_min);
        buffer_len += 5;
        
        sprintf(buffer + buffer_len, " - ");
        buffer_len += 3;

        sprintf(buffer + buffer_len, "%s\"", event.summary);
        buffer_len += strlen(event.summary) + 1;

        if (i < events.length - 1) {
            buffer[buffer_len] = ',';
            buffer_len++;
        }
    }

    buffer[buffer_len] = ']';

    return buffer;
}

char* dump_schedule_state(Schedule schedule_widget, bool active) {
    char date_buffer[16];
    bzero(date_buffer, 16);

    format_calendartxt_date(date_buffer, schedule_widget.year, schedule_widget.month, schedule_widget.day);

    char* events_json = dump_events(schedule_widget.events); 

    size_t buff_len = strlen(events_json) + strlen(date_buffer) + 200;
    char* json_buffer = malloc(sizeof(char) * buff_len);
    bzero(json_buffer, buff_len);
    sprintf(
        json_buffer, 
        "\"schedule_widget\": {\"current_date\": \"%s\", \"selected_event\": %d, \"events\": %s, \"active\": %s}", 
        date_buffer,
        schedule_widget.selected_event,
        events_json,
        active ? "true" : "false"
    );

    free(events_json);
    events_json = NULL;

    return json_buffer;
}

char* dump_calendar_state(Calendar calendar_widget, bool active) {
    char date_buffer[16];
    bzero(date_buffer, 16);

    format_calendartxt_date(date_buffer, calendar_widget.year,
        calendar_widget.month, calendar_widget.selected_day
    );

    size_t buff_len = strlen(date_buffer) + 200;
    char* json_buffer = malloc(sizeof(char) * buff_len);
    bzero(json_buffer, buff_len);
    sprintf(
        json_buffer,
        "\"calendar_widget\": {\"current_date\": \"%s\", \"active\": %s}",
        date_buffer,
        active ? "true" : "false"
    );

    return json_buffer;
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

void* state_broadcast(void* args) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        debug_log("failed to get socket fd.\n");
        close(fd);
        return NULL;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    int r = bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    if (r != 0) {
        debug_log("Failed to bind socket to %s\n", SOCKET_PATH);
        return NULL;
    }

    listen(fd, 5);
    int conn = accept(fd, NULL, NULL);

    size_t n;
    do {
        char* message = channel_receive(broadcast_channel, &n);
        write(conn, message, n);
        free(message);
        message = NULL;
    } while (true);

    close(fd);
    return NULL;
}

pthread_t start_state_broadcast() {
    pthread_t broadcaster;
    pthread_create(&broadcaster, NULL, state_broadcast, NULL);

    return broadcaster;
}
