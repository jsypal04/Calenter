#include <libnotify/notify.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libnotify/notification.h>
#include "../common/calendartxt.h"

void error(const char* msg);

void* worker(void* args);
void  listener();

int main() {
    notify_init("Calenter");
    pthread_t worker_thread;
    int retval;

    retval = pthread_create(&worker_thread, NULL, worker, NULL);
    if (retval != 0) {
        error("Failed to start the notification daemon.\n");
    }

    pthread_join(worker_thread, NULL);
    notify_uninit();

    return 0;
}

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void* worker(void* args) {
    #define MSG_LEN 1024

    int diff_s;
    char msg[MSG_LEN] = "\0";
    time_t n_time, event_time;
    struct tm tm_time    = {0};
    struct events events = {0};
    struct event  event  = {0};

    do {
        n_time = time(NULL);
        localtime_r(&n_time, &tm_time);
    } while (tm_time.tm_sec != 0);

    printf("Notification daemon synced to the minute.\n");

    while (true) {
        printf("Polling events...\n");
        n_time = time(NULL);
        localtime_r(&n_time, &tm_time);

        events = get_events(
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday);

        printf("events.length = %zu\n", events.length);

        for (int i = 0; i < events.length; i++) {
            event = events.events[i];
            event_time = mktime(&event.datetime);
            diff_s = (int)difftime(event_time, n_time);

            printf("diff_s = %d\n", diff_s);

            if (diff_s <= (9.5 * 60) || diff_s >= (10.5 * 60)) {
                printf("event not in the appropriate time frame.\n");
                continue;
            }

            strcpy(msg, "'");
            strncpy(msg + 1, event.summary, 100);
            if (strlen(event.summary) <= 100) {
                strcpy(msg + strlen(event.summary), "' begins in 10 minutes.");
            } else {
                strcpy(msg + 100, "' begins in 10 minutes.");
            }

            NotifyNotification* noti = notify_notification_new(
                "Calenter", msg, "dialog-information");
            notify_notification_show(noti, NULL);
            g_object_unref(G_OBJECT(noti));
        }

        free_events(events);
        sleep(60);
    }

    return NULL;
}
