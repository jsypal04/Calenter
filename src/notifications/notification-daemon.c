#include <libnotify/notify.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <libnotify/notification.h>
#include <unistd.h>
#include <fcntl.h>
#include "../common/calendartxt.h"

#define KILL        'k'
#define NOTIFY_TIME 't'

#define DAEMON_FIFO "/tmp/calenter-notification-daemon.fifo"

int  notify_time = 10;
bool running     = true;

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

    listener();

    running = false;

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

    while (running) {
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

            int lower_bound = notify_time * 60 - 30;
            int upper_bound = notify_time * 60 + 30;
            if (diff_s <= lower_bound || diff_s >= upper_bound) {
                printf("event not in the appropriate time frame.\n");
                continue;
            }

            char msg_suffix[64] = "\0";
            sprintf(msg_suffix, "' begins in %d minutes.", notify_time);

            strcpy(msg, "'");
            strncpy(msg + 1, event.summary, 100);
            if (strlen(event.summary) <= 100) {
                strcpy(msg + strlen(event.summary) + 1, msg_suffix);
            } else {
                strcpy(msg + 100, msg_suffix);
            }

            NotifyNotification* noti = notify_notification_new(
                "Calenter", msg, "");
            notify_notification_show(noti, NULL);
            g_object_unref(G_OBJECT(noti));
        }

        free_events(events);
        sleep(60 - tm_time.tm_sec);
    }

    return NULL;
}

void listener() {
    int fifo_fd, n;
    char buf[128] = "\0";

    do {
        fifo_fd = open(DAEMON_FIFO, O_RDONLY);

        bzero(buf, sizeof(buf));
        n = read(fifo_fd, buf, sizeof(buf) - 1);
        buf[n] = '\0';

        printf("Received: '%s'\n", buf);
        for (int i = 0; i < sizeof(buf) - 1; i++) {
            printf("%d ", buf[i]);
        }
        printf("\n");

        switch (buf[0]) {
            case KILL:
                close(fifo_fd);
                unlink(DAEMON_FIFO);
                return;
            case NOTIFY_TIME:
                notify_time = atoi(buf + 1);
                printf("notify_time: %d\n", notify_time);
                break;
        }

        close(fifo_fd);
    } while (true);
}
