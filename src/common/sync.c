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
#include "calendartxt.h"
#include "ics.h"
#include "array.h"
#include "debug.h"


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


pthread_t syncer_thread;

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

void sync_calendar_wrapper(Array* remote_urls) {
    pthread_create(&syncer_thread, NULL, sync_calendar_curl, array_dup(remote_urls));
}

void* sync_calendar_curl(void* ptr) {
    Array* remote_urls = (Array*)ptr;

    CURL* curl;
    FILE* output_file;
    CURLcode res;

    curl = curl_easy_init();

    if (!curl) {
        curl_easy_cleanup(curl);
        return NULL;
    };


    for (int i = 0; i < array_len(remote_urls); i++) {
        char* remote_url = get_string(remote_urls, i);
        output_file = fopen(SYNC_TMP_FILE, "w");

        debug_log("remote = %s\n", remote_url);

        curl_easy_setopt(curl, CURLOPT_URL, remote_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            debug_log("curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            return NULL;
        }

        fclose(output_file);

        update_calendartxt(SYNC_TMP_FILE);
    }


    NotifyNotification* noti = notify_notification_new(
        "Sync Successful",
        "Calenter successfully sank with your Google Calendar.",
        ""
    );
    notify_notification_show(noti, NULL);
    g_object_unref(G_OBJECT(noti));
    curl_easy_cleanup(curl);

    free_array(remote_urls);

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

// Might want to make this function public
int update_calendartxt(char* ics_file) {
    struct events events = parse_ics(ics_file);
    if (events.length == 0) return NO_EVENTS;

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

    // Write all events to calendar.txt
    
    time_t raw_time = time(NULL);
    struct tm* today = localtime(&raw_time);
    
    for (int i = 0; i < events.length; i++) {
        struct event event = events.events[i];

        if (
            date_cmp(
                today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
                event.datetime.tm_year + 1900, event.datetime.tm_mon + 1, event.datetime.tm_mday
            ) > 0
        ) {
            continue;
        }

        struct events current_events = get_events(
            event.datetime.tm_year + 1900, 
            event.datetime.tm_mon + 1, 
            event.datetime.tm_mday
        );
        
        bool duplicate_found = false;
        for (int i = 0; i < current_events.length; i++) {
            struct event current_event = current_events.events[i];
            
            // TODO: refactor this condition. I don't like it.
            if (
                current_event.all_day && event.all_day && 
                strcmp(current_event.summary, event.summary) == 0
            ) {
                duplicate_found = true;
                break;
            } else if (
                time_cmp(current_event.datetime.tm_hour, current_event.datetime.tm_min, event.datetime.tm_hour, event.datetime.tm_min) == 0 &&
                strcmp(current_event.summary, event.summary) == 0
            ) {
                duplicate_found = true;
                break;
            }
        }

        if (duplicate_found) continue;
        

        debug_log("current_events.length = %d\n", current_events.length);

        add_event(event, event.datetime.tm_year + 1900,
                event.datetime.tm_mon + 1, event.datetime.tm_mday);
    }

    return SUCCESS;
}
