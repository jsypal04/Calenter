#ifndef SYNC_H
#define  SYNC_H

#include "types.h"

int  sync_calendar(char* remote_url);
void sync_calendar_wrapper(Array* remote_urls);

#endif
