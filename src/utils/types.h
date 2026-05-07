#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <stdbool.h>

enum type {
    INT,
    FLOAT,
    STRING,
    DATETIME
};

typedef struct array_t Array;

enum FREQ {
    NONE,
    MINUTELY,
    HOURLY,
    DAILY,
    WEEKLY,
    MONTHLY,
    YEARLY
};

enum WKDAY {
    SU,
    MO,
    TU,
    WE,
    TH,
    FR,
    SA
};

struct rrule {
    enum FREQ freq;
    struct tm until;
    int count;
    int interval;
    Array* bysecond;
    Array* byminute;
    Array* byhour;
    Array* byday;
    Array* bymonthday;
    Array* byyearday;
    Array* byweekno;
    Array* bymonth;
    Array* bysetpos;
    enum WKDAY wkst;
};

// for all day events, hour == min == -1
struct event {
    bool all_day;
    struct tm datetime;
    char* summary;
    struct rrule rrule;
};

struct events {
  size_t size;
  size_t length;
  struct event* events;
};


#endif
