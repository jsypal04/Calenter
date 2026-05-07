#ifndef CALENDARTXT_H
#define CALENDARTXT_H

#include <stddef.h>
#include <time.h>
#include "../utils/types.h"

/*
 * Gets an array of all the events for a given day from calendar.txt
 */
struct events get_events(int year, int month, int day);

/*
 * Writes the event to calendar.txt. Returns 0 on success, -1 on failure.
 */
int add_event(struct event event, int year, int month, int day);

/*
 * Generates an array of events based on a single repeating event
 *
 * TODO: Not implemented yet.
 * */
struct events expand_rrule(struct event event);

/*
 * Deletes an event from calendar.txt. Returns 0 on success, -1 on failure.
 */
int delete_event(struct event event);

/*
 * Inializes an empty events array with initial size of 10
 */
void init_events(struct events* events);


/*
 * Returns the index of the first occurence of event in events. Returns -1 if not found.
 */
int find_event(struct events* events, struct event event);

/*
 * Adds the event to the end of the events array
 */
void append_event(struct events* events, struct event new_event);

/*
 * Inserts event into events preserving chronological order
 */
void insert_event(struct events* events, struct event new_event);


/*
 * Frees the fields that are dynamically allocated in get events
 */
void free_events(struct events events);


/*
 * Returns -1 if time 1 is before time 2, 1 if time 1 is after time 2
 * and 0 if they are the same
 */
int time_cmp(int hour1, int min1, int hour2, int min2);

/*
 * Returns -1 if date 1 is before date 2, 1 if date 1 is after date 2
 * and 0 if they are the same
 */
int date_cmp(int year1, int month1, int day1,
             int year2, int month2, int day2);

/*
 * Formats the given date for calendar.txt: "yyyy-mm-dd"
 */
void format_calendartxt_date(char* buffer, int year, int month, int day);

/*
 * Formats the time in 24-hour format like so: "HH:MM".
 * `hour = -1` indicates an all day event formatted like: "ALL DAY".
 *
 * Note: `buffer` should be at least 7 characters long.
 */
void format_time(char* buffer, int hour, int min);

#endif
