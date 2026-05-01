/*
 * sync.c
 *
 * This file provides a single function for downloading
 * the ics file from the url specified in the config file.
 * Currently it uses the python/bash script setup that I
 * developed initially but I would like to migrate that over
 * to a C version.
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include "sync.h"
#include "calendartxt.h"
#include "config.h"
#include "ics.h"

#define SYNC_SCRIPT "fetch_calendar.bash"
#define SYNC_SCRIPT_PATH "/.calendar/scripts/fetch_calendar.bash"
#define SYNC_TMP_FILE "/tmp/gcal.ics"

typedef enum _ERRNO {
    SUCCESS,
    NO_SYNC_SCRIPT_PATH,
    NO_REMOTE,
    CURL_FAILED,
} SYNC_ERR;

char* get_sync_script_path();
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream);

int sync_calendar() {
    Config config = read_config();
    if (config.remote_url == NULL) return NO_REMOTE;

    char* sync_script_path = get_sync_script_path();

    if (sync_script_path == NULL) return NO_SYNC_SCRIPT_PATH;

    if (fork() == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        execl(sync_script_path, SYNC_SCRIPT, config.remote_url, NULL);
    }

    free(config.remote_url);

    return SUCCESS;
}

int sync_calendar_curl() {
    CURL* curl;
    FILE* output_file;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return CURL_FAILED;
    };

    Config config = read_config();
    if (config.remote_url == NULL) return NO_REMOTE;

    output_file = fopen(SYNC_TMP_FILE, "w");

    curl_easy_setopt(curl, CURLOPT_URL, config.remote_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) return CURL_FAILED;


    fclose(output_file);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    struct events events = parse_ics(SYNC_TMP_FILE);

    return SUCCESS;
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
