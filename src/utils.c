#include <stdio.h>
#include <time.h>
#include <string.h>
#include "calenter.h"

// TODO: Handle leap years.
int get_days_in_month(int month) {
    switch (month) {
        case 1: return 31;
        case 2: return 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return -1;
    };
}

char* get_month_name(int month) {
    switch (month) {
        case 1: return "January";
        case 2: return "February";
        case 3: return "March";
        case 4: return "April";
        case 5: return "May";
        case 6: return "June";
        case 7: return "July";
        case 8: return "August";
        case 9: return "September";
        case 10: return "October";
        case 11: return "November";
        case 12: return "December";
        default: return NULL;
    };
}

char* get_wday_name(int wday) {
    switch (wday) {
        case 0: return "Sunday";
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
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
