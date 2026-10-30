#include <libnotify/notification.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// TODO: Fix the warings in this file.

// Allocate at least 11 bytes for this.
void fmt_ISO_8601_date(char* buffer, struct tm* t) {
    char  year[16] = "\0";
    char month[16] = "\0";
    char   day[16] = "\0";

    // Year is always four digits until the year 10,000 :)
    sprintf(year, "%d", t->tm_year + 1900);

    if (t->tm_mon + 1 < 10) {
        sprintf(month, "0%d", t->tm_mon + 1);
    } else {
        sprintf(month, "%d", t->tm_mon + 1);
    }

    if (t->tm_mday < 10) {
        sprintf(day, "0%d", t->tm_mday);
    } else {
        sprintf(day, "%d", t->tm_mday);
    }

    sprintf(buffer, "%s-%s-%s", year, month, day);
}

// I recomend making sure buffer has size 32 or so.
void fmt_ISO_8601_timestamp(char* buffer, struct tm* t, bool tz) {
    char   date[16] = "\0";
    char   hour[16] = "\0";
    char    min[16] = "\0";
    char    sec[16] = "\0";
    char gmtoff[16] = "\0";

    fmt_ISO_8601_date(date, t);

    if (t->tm_hour < 10) {
        sprintf(hour, "0%d", t->tm_hour);
    } else {
        sprintf(hour, "%d", t->tm_hour);
    }

    if (t->tm_min < 10) {
        sprintf(min, "0%d", t->tm_min);
    } else {
        sprintf(min, "%d", t->tm_min);
    }

    if (t->tm_sec < 10) {
        sprintf(sec, "0%d", t->tm_sec);
    } else {
        sprintf(sec, "%d", t->tm_sec);
    }

    // TODO: gmt offset is not always on the hour. Fix this eventually.
    int gmtoff_h = floor((double) t->tm_gmtoff / 3600.0);
    int abs_gmtoff_h = abs((int) gmtoff_h);
    if (abs_gmtoff_h < 10) {
        if (gmtoff_h < 0) {
            sprintf(gmtoff, "-0%d:00", abs_gmtoff_h);
        } else {
            sprintf(gmtoff, "0%d:00", gmtoff_h);
        }
    } else {
        sprintf(gmtoff, "%d:00", gmtoff_h);
    }

    if (tz) {
        sprintf(buffer, "%sT%s:%s:%s%s", date, hour, min, sec, gmtoff);
    } else {
        sprintf(buffer, "%sT%s:%s:%sZ", date, hour, min, sec);
    }
}

void trim(char* str) {
    int index1 = 0;
    int index2 = 0;

    while (isspace(str[index1])) index1++;

    while (index1 < strlen(str)) {
        str[index2] = str[index1];
        index1++;
        index2++;
    }

    while (index2 < strlen(str)) {
        str[index2] = '\0';
        index2++;
    }

    index1 = strlen(str) - 1;
    while (isspace(str[index1])) {
        str[index1] = '\0';
        index1--;
    }
}

// NOTE: this function DOES NOT check that the string consists of alphabet
// characters. If you pass a string with non-alpha chars weird things will
// happen. You have been warned.
void to_lower(char* str) {
    char c;
    for (int i = 0; i < strlen(str); i++) {
        c = str[i];
        if (c >= 97) continue;
        str[i] = c + 32;
    }
}
