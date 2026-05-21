#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <stdbool.h>

typedef struct array_t Array;

typedef struct {
    enum {
        BYMONTH,
        BYWEEKNO,
        BYYEARDAY,
        BYMONTHDAY,
        BYDAY,
        BYHOUR,
        BYMINUTE,
        BYSECOND,
        BYSETPOS
    } type;
    Array* values;
} BYxxx_Rule;

enum type {
    INT,
    FLOAT,
    STRING,
    BYXXX_RULE,
};

union element {
    int i;
    float f;
    char* s;
    BYxxx_Rule b;
};


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
    Array* BYxxx_Rules;
    enum WKDAY wkst;
};

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
