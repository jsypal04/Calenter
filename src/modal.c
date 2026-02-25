#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "calenter.h"

enum active_input {
    HOUR,
    MIN,
    SUMMARY,
};

typedef struct _input_fields {
    enum active_input active_input;
    WINDOW* summary_win;
    int hour_index;
    int min_index;
    int summary_index;
    char hour[5];
    char min[5];
    char summary[2000];
} Inputs;


void set_byte(Inputs* inputs, char ch);
void delete_byte(Inputs* inputs);
void render_input_fields(WINDOW* win, Inputs* inputs);

struct event add_event_modal(Window** windows, struct event* event) {
    int height = 3 * LINES / 4;
    int width = COLS / 2;

    WINDOW* modal = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    keypad(modal, true);
    werase(modal);
    box(modal, 0, 0);
    mvwprintw(modal, 0, 1, " Add Event ");

    Inputs inputs = {0};
    inputs.active_input = HOUR;
    inputs.summary_win = derwin(modal, 10, width - 6, 7, 3);

    if (event != NULL) {
        assert(event->hour < 24);
        assert(event->min < 60);
        assert(strlen(event->summary) < 2000);

        char* hour_fmt_str = (event->hour < 10) ? "0%d" : "%d";
        char* min_fmt_str = (event->min < 10) ? "0%d" : "%d";

        sprintf(inputs.hour, hour_fmt_str, event->hour);
        sprintf(inputs.min, min_fmt_str, event->min);
        sprintf(inputs.summary, "%s", event->summary);

        inputs.hour_index = strlen(inputs.hour);
        inputs.min_index = strlen(inputs.min);
        inputs.summary_index = strlen(inputs.summary);
    } else {
        strcpy(inputs.hour, "  ");
        strcpy(inputs.min, "  ");
    }

    render_input_fields(modal, &inputs);
    wrefresh(modal);

    int ch = getch();
    while (ch != 10 && ch != 27) {
        switch (ch) {
            case 127: {
                delete_byte(&inputs);
                render_input_fields(modal, &inputs);
                break;
            }
            case '\t': {
                inputs.active_input = (inputs.active_input + 1) % 3;
                break;
            }
            default: {
                set_byte(&inputs, ch);
                render_input_fields(modal, &inputs);
            }
        }

        ch = getch();
    }

    struct event new_event = {0};

    if (ch == 10) {
        new_event.hour = atoi(inputs.hour);
        new_event.min = atoi(inputs.min);
        new_event.summary = strdup(inputs.summary);
    }

    werase(modal);
    delwin(modal);

    for (int i = 0; i < NUM_WINDOWS; i++) {
        if (i == SCHEDULE_WIN) {
            refresh_win(windows[i], true);
            continue;
        }

        refresh_win(windows[i], false);
    }

    return new_event;
}

void set_byte(Inputs* inputs, char ch) {
    if (inputs->active_input == HOUR) {
        if (inputs->hour_index >= 2 || ch < 48 || ch > 57) return;

        inputs->hour[inputs->hour_index] = ch;
        inputs->hour_index++;

    } else if (inputs->active_input == MIN) {
        if (inputs->min_index >= 2 || ch < 48 || ch > 57) return;

        inputs->min[inputs->min_index] = ch;
        inputs->min_index++;
    } else {
        if (inputs->summary_index >= 1999) return;

        inputs->summary[inputs->summary_index] = ch;
        inputs->summary_index++;
    }
}

void delete_byte(Inputs* inputs) {
    switch (inputs->active_input) {
        case HOUR: {
            if (inputs->hour_index > 0) inputs->hour_index--;
            inputs->hour[inputs->hour_index] = ' ';
            break;
        }
        case MIN: {
            if (inputs->min_index > 0) inputs->min_index--;
            inputs->min[inputs->min_index] = ' ';
            break;
        }
        case SUMMARY: {
            if (inputs->summary_index > 0) inputs->summary_index--;
            inputs->summary[inputs->summary_index] = ' ';
            break;
        }
    };
}

void render_input_fields(WINDOW* win, Inputs* inputs) {
    mvwprintw(win, 2, 3, "Time (24 hour):");
    wattron(win, COLOR_PAIR(INPUT_FIELD_PAIR));
    mvwprintw(win, 3, 3, "%s", inputs->hour);
    wattroff(win, COLOR_PAIR(INPUT_FIELD_PAIR));

    mvwprintw(win, 3, 5, ":");

    wattron(win, COLOR_PAIR(INPUT_FIELD_PAIR));
    mvwprintw(win, 3, 6, "%s", inputs->min);
    wattroff(win, COLOR_PAIR(INPUT_FIELD_PAIR));

    mvwprintw(win, 6, 3, "Summary (2000 character limit):");
    mvwprintw(inputs->summary_win, 0, 0, "%s", inputs->summary);
    wbkgd(inputs->summary_win, COLOR_PAIR(INPUT_FIELD_PAIR));

    wrefresh(win);
    wrefresh(inputs->summary_win);
}
