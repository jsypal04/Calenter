/**
 * json.c
 *
 * This file stringifies the UI state and broadcasts it over a unix domain
 * socket. It has no functionality to parse json
 * */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/calendartxt.h"
#include "calenter.h"
#include "../common/debug.h"

#define SOCKET_PATH "/tmp/calenter.sock"

char* dump_events(struct events events) {
    int buffer_cap = 2028;
    int buffer_len = 0;
    char* buffer = malloc(sizeof(char) * buffer_cap);
    bzero(buffer, sizeof(char) * buffer_cap);

    buffer[buffer_len] = '[';
    buffer_len++;

    for (int i = 0; i < events.length; i++) {
        if (buffer_len == buffer_cap) {
            buffer_cap *= 2;
            char* new_buffer = malloc(sizeof(char) * buffer_cap);
            bzero(new_buffer, sizeof(char) * buffer_cap);
            strncpy(new_buffer, buffer, buffer_cap);
            free(buffer);
            buffer = new_buffer;
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

char* dump_ui_state(Schedule schedule_widget, Calendar calendar_widget) {
    char date_buffer[16];
    bzero(date_buffer, 16);

    format_calendartxt_date(date_buffer, schedule_widget.year, schedule_widget.month, schedule_widget.day);

    char* events_json = dump_events(schedule_widget.events); 

    char* json_buffer = malloc(sizeof(char) * (strlen(events_json) + strlen(date_buffer) + 200));
    sprintf(
        json_buffer, 
        "{\"schedule_widget\": {\"current_date\": \"%s\", \"selected_event\": %d, \"events\": %s}}", 
        date_buffer,
        schedule_widget.selected_event,
        events_json
    );

    debug_log("%s\n", json_buffer);

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

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        debug_log("failed to connect to socket %s\n", SOCKET_PATH);
        close(fd);
        return NULL;
    }

    debug_log("Connected to harness on %s\n", SOCKET_PATH);

    // while state received from channel that I will create
    //     write(fd, message, strlen(message) - 1)
    //     free(message);
    //     message = NULL;

    close(fd);
    return NULL;
}

pthread_t start_state_broadcast() {
    pthread_t broadcaster;
    pthread_create(&broadcaster, NULL, state_broadcast, NULL);

    return broadcaster;
}
