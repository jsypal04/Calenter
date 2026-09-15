#include <stdlib.h>
#include <time.h>

#include "../calenter.h"

Calendar* new_ui_calendar() {
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    Calendar* cal = malloc(sizeof(Calendar));
    cal->selected_day = info->tm_mday;
    cal->month = info->tm_mon + 1;
    cal->year = info->tm_year + 1900;

    return cal;
}

void free_ui_calendar(Calendar* calendar) {
    free(calendar);
    calendar = NULL;
}

// void render_ui_calendar(UIPane* pane, UIObject* object) {
//
// }
