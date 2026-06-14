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

typedef enum ui_unit_t {
    CELLS = 0,
    PERCENT = 1,
} UIUnit;


typedef struct ui_layout_t {
    int    height;
    int    width;
    Array* layout_objs;
} UILayout;

typedef struct ui_layout_obj_t {
    int       id;
    float     r_height;
    float     r_width;
    float     r_starty;
    float     r_startx;
    UIUnit    unit;
    UILayout* layout;
} UILayoutObj;

enum type {
    INT           = 0,
    FLOAT         = 1,
    STRING        = 2,
    BYXXX_RULE    = 3,
    UI_LAYOUT_OBJ = 4,
};

union element {
    int         i;
    float       f;
    char*       s;
    BYxxx_Rule  b;
    UILayoutObj lo;
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
