#ifndef GCAL_H
#define GCAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "types.h"

typedef struct gcal GCal;

GCal* new_gcal_client(bool silence);

void destroy_gcal_client(GCal* gcal);

int gcal_authenticate(GCal* gcal);

int gcal_get_calendar_id(GCal* gcal, char** calendar_id);

int gcal_insert_event(GCal* gcal, struct event event);

#ifdef __cplusplus
}
#endif

#endif
