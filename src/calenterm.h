#ifndef CALENTERM_H
#define CALENTERM_H

#include <stddef.h>

struct event {
  int year;
  int month;
  int day;
  int hour;
  int min;
  char* summary;
};

struct events {
  size_t size;
  size_t length;
  struct event* events;
};

struct events get_events(int year, int month, int day);
void add_event(struct event event, int year, int month, int day);

void init_events(struct events* events);
void append_event(struct events* events, struct event new_event);

#endif
