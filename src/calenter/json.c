/**
 * json.c
 *
 * This file stringifies the UI state and broadcasts it over a unix domain
 * socket. It has no functionality to parse json
 * */

#include <semaphore.h>
#include <math.h>
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

// #define CHANNEL_BUFFER_CAP 8
//
// typedef struct channel {
//     char buffer[CHANNEL_BUFFER_CAP];
//     bool last_chunk;
//     sem_t semaphore;
//     pthread_mutex_t mutex;
// } Channel;
//
// Channel broadcast_channel = {0};
//
// void init_channel(Channel* channel) {
//     bzero(channel->buffer, CHANNEL_BUFFER_CAP);
//     channel->last_chunk = false;
//     sem_init(&channel->semaphore, 0, 0);
// }
//
// void destroy_channel(Channel* channel) {
//     sem_destroy(&channel->semaphore);
// }

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

// void channel_send(Channel* channel, char* message) {
//     if (strlen(message) < CHANNEL_BUFFER_CAP) {
//         pthread_mutex_lock(&channel->mutex);
//         strncpy(channel->buffer, message, CHANNEL_BUFFER_CAP - 1);
//         channel->last_chunk = true;
//         pthread_mutex_unlock(&channel->mutex);
//         sem_post(&channel->semaphore);
//         return;
//     }
//
//     double num_chunks = ceil((double) strlen(message) / (CHANNEL_BUFFER_CAP - 1));
//     int chunk_number = 1;
//
//     for (int i = 0; i < strlen(message); i += CHANNEL_BUFFER_CAP - 1) {
//         sem_wait(&channel->semaphore);
//         char* chunk_start = message + i;
//         pthread_mutex_lock(&channel->mutex);
//         strncpy(channel->buffer, chunk_start, CHANNEL_BUFFER_CAP - 1);
//
//         if (chunk_number == num_chunks) {
//             channel->last_chunk = true;
//         } else {
//             channel->last_chunk = false;
//         }
//
//         // This should never happen but just in case, log it and correct the issue.
//         if (channel->buffer[CHANNEL_BUFFER_CAP - 1] != '\0') {
//             debug_log("WARNING: channel buffer is not null terminated. Inserting a null character.\n");
//             channel->buffer[CHANNEL_BUFFER_CAP - 1] = '\0';
//         }
//         pthread_mutex_unlock(&channel->mutex);
//
//         sem_post(&channel->semaphore);
//     }
// }
//
//
// char* channel_receive(Channel* channel) {
//     size_t message_cap = CHANNEL_BUFFER_CAP;
//     size_t message_len = 0;
//     char* message = malloc(sizeof(char) * message_cap);
//     bzero(message, message_cap);
//
//     while (true) {
//         sem_wait(&channel->semaphore);
//
//         // receive the message chunk
//         pthread_mutex_lock(&channel->mutex);
//         size_t chunk_len = strlen(channel->buffer);
//         if (message_len + chunk_len >= message_cap - 1)
//             // Since this function doubles the message buffer and the message buffer
//             // starts with the same capacity as the channel buffer I think we are
//             // guarenteed that the next chunk will fit after one doubling.
//             expand_buffer(message, message_len, message_cap);
//
// #undef printf
//         printf("\nchunk = %s\n", channel->buffer);
//
//         strncpy(message + message_len, channel->buffer, message_cap - message_len - 1);
//         message_len += strlen(channel->buffer);
//
//         bzero(channel->buffer, CHANNEL_BUFFER_CAP);
//
//         if (channel->last_chunk) {
//             pthread_mutex_unlock(&channel->mutex);
//             break;
//         }
//         pthread_mutex_unlock(&channel->mutex);
//
//         sem_post(&channel->semaphore);
//     }
//
//     return message;
// }


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
