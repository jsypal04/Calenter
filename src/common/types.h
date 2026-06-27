#ifndef TYPES_H
#define TYPES_H

#include <time.h>
#include <stdbool.h>
#include <ncurses.h>

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

typedef struct ui_layout_t UILayout;
typedef struct ui_object_t UIObject;

typedef void (*Renderer)(UIObject*);
typedef void (*Resizer)(UILayout*, UIObject*);

typedef struct ui_float_t {
    float value;
    UIUnit unit;
} UIFloat;

typedef enum layouts_t {
    ROW,
} LayoutType;

typedef struct ui_layout_t {
    int        height;
    int        width;
    LayoutType layout_type;
    Array*     layout_objs;
} UILayout;

typedef struct pane_t {
    bool      is_active;
    UILayout* layout;
    WINDOW*   win;
    char*     title;
} UIPane;

typedef struct ui_object_t {
    int           id;
    enum componant {
        PANE = 0,
    }      componant;
    UIFloat   height;
    UIFloat    width;
    UIFloat   startx;
    UIFloat   starty;
    union {
        UIPane* pane; // This is sometimes NULL
    }           data;
    Renderer  render;
    Resizer   resize;
} UIObject;

enum type {
    INT           = 0,
    FLOAT         = 1,
    STRING        = 2,
    BYXXX_RULE    = 3,
    UI_OBJECT     = 4,
};

union element {
    int         i;
    float       f;
    char*       s;
    BYxxx_Rule  b;
    UIObject* uio;
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
