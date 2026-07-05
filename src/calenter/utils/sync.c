/*
 * sync.c
 *
 * This file provides a single function for downloading
 * the ics file from the url specified in the config file.
 * Currently it uses the python/bash script setup that I
 * developed initially but I would like to migrate that over
 * to a C version.
 * */


#include <libnotify/notification.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include "glib-object.h"


#include "sync.h"
#include "debug.h"
#include "../../common/calendartxt.h"
#include "../ics/ics.h"

#define SYNC_SCRIPT "fetch_calendar.bash"
#define SYNC_SCRIPT_PATH "/.calendar/scripts/fetch_calendar.bash"
#define SYNC_TMP_FILE "/tmp/gcal.ics"

typedef enum _ERRNO {
    SUCCESS,
    NO_SYNC_SCRIPT_PATH,
    SCRIPT_FAILED,
    NO_REMOTE,
    CURL_FAILED,
    NO_EVENTS,
} SYNC_ERR;

char*  get_sync_script_path();
void*  sync_calendar_curl(void* ptr);
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream);
int    update_calendartxt(char* ics_file);

int sync_calendar(char* remote_url) {
    char* sync_script_path = get_sync_script_path();

    if (sync_script_path == NULL) return NO_SYNC_SCRIPT_PATH;

    int pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        execl(sync_script_path, SYNC_SCRIPT, remote_url, NULL);
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        return EXIT_SUCCESS;
    }

    return SCRIPT_FAILED;
}

void sync_calendar_wrapper(char* remote_url) {
    pthread_t syncer_thread;
    pthread_create(&syncer_thread, NULL, sync_calendar_curl, remote_url);
}

void* sync_calendar_curl(void* ptr) {
    char* remote_url = (char*)ptr;

    CURL* curl;
    FILE* output_file;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return NULL;
    };

    output_file = fopen(SYNC_TMP_FILE, "w");

    curl_easy_setopt(curl, CURLOPT_URL, remote_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) return NULL;


    fclose(output_file);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    debug_log("About to run update_calendartxt\n");
    update_calendartxt(SYNC_TMP_FILE);
    debug_log("Ran update_calendartxt\n");

    NotifyNotification* noti = notify_notification_new(
        "Sync Successful",
        "Calenter successfully sank with your Google Calendar.",
        ""
    );
    notify_notification_show(noti, NULL);
    g_object_unref(G_OBJECT(noti));
    return NULL;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE *fp = (FILE *)stream;
    return fwrite(ptr, size, nmemb, fp);
}

/*
 * Gets the absolute path to the sync script
 * */
char* get_sync_script_path() {
    char* home = getenv("HOME");

    if (home == NULL) return NULL;

    int length = strlen(home) + strlen(SYNC_SCRIPT_PATH) + 5;

    char* sync_script_path = malloc(length * sizeof(char));
    memset(sync_script_path, '\0', length * sizeof(char));
    sprintf(sync_script_path, "%s%s", home, SYNC_SCRIPT_PATH);

    return sync_script_path;
}

int update_calendartxt(char* ics_file) {
    debug_log("running line %d\n", __LINE__);
    struct events events = parse_ics(ics_file);
    if (events.length == 0) return NO_EVENTS;
    debug_log("running line %d\n", __LINE__);

    // for (int i = 0; i < events.length; i++) {
    //     struct event event = events.events[i];
    //     printf("%d-%d-%d %d:%d - %s\n",
    //             event.datetime.tm_mon + 1, event.datetime.tm_mday, event.datetime.tm_year + 1900,
    //             event.datetime.tm_hour, event.datetime.tm_min, event.summary
    //     );
    // }
    // printf("---------------------\n");

    debug_log("running line %d\n", __LINE__);
    for (int i = 0; i < events.length; i++) {
        struct event event = events.events[i];
        if (event.rrule.freq == NONE) continue;

        remove_event(&events, event);

        struct events expanded_event = expand_rrule(event);
        for (int j = 0; j < expanded_event.length; j++) {
            insert_event(&events, expanded_event.events[j]);
        }
        free(expanded_event.events);
        expanded_event.events = NULL;
    }
    debug_log("running line %d\n", __LINE__);

    // for (int i = 0; i < events.length; i++) {
    //     struct event event = events.events[i];
    //     printf("%d-%d-%d %d:%d - %s\n",
    //             event.datetime.tm_mon + 1, event.datetime.tm_mday, event.datetime.tm_year + 1900,
    //             event.datetime.tm_hour, event.datetime.tm_min, event.summary
    //     );
    // }

    // Write all events to calendar.txt

    debug_log("length: %d\n", events.length);
    for (int i = 0; i < events.length; i++) {
        debug_log("i: %d\n", i);
        struct event event = events.events[i];
        add_event(event, event.datetime.tm_year + 1900,
                event.datetime.tm_mon + 1, event.datetime.tm_mday);
    }
    debug_log("running line %d\n", __LINE__);

    return SUCCESS;
}
