#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "calenter.h"
#include "drivers/calendartxt.h"

enum input_tag {
    HOUR,
    MIN,
    SUFFIX,
    SUMMARY,
};

typedef struct _input_field {
    WINDOW* win;
    int index;
    char content[2048];
} InputField;

typedef struct _input_fields {
    bool           all_day; 
    enum input_tag active_input;
    InputField     hour;
    InputField     min;
    InputField     suffix;
    InputField     summary;
} Inputs;


void set_byte(Inputs* inputs, char ch);
void delete_byte(Inputs* inputs);
void render_input_fields(WINDOW* win, Inputs* inputs);
void init_inputs(Inputs* inputs, WINDOW* modal, int width);

struct event add_event_modal(Window** windows, struct event* event) {
    int height = 3 * LINES / 4;
    int width = COLS / 2;

    WINDOW* modal = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    keypad(modal, true);
    box(modal, 0, 0);

    Inputs inputs;
    init_inputs(&inputs, modal, width);

    if (event != NULL) {
        assert(event->hour < 24);
        assert(event->min < 60);
        assert(strlen(event->summary) < 2000);

        if (event->hour >= 12) {
            strcpy(inputs.suffix.content, "PM");
        } else {
            strcpy(inputs.suffix.content, "AM");
        }

        int hour = event->hour != 12 ? event->hour % 12 : 12;

        char* hour_fmt_str = (hour < 10) ? "0%d" : "%d";
        char* min_fmt_str = (event->min < 10) ? "0%d" : "%d";

        if (event->hour == -1) {
            inputs.all_day = true;
            inputs.active_input = SUMMARY;
        } else {
            sprintf(inputs.hour.content, hour_fmt_str, hour);
            sprintf(inputs.min.content, min_fmt_str, event->min);

            inputs.hour.index = strlen(inputs.hour.content);
            inputs.min.index = strlen(inputs.min.content);
        }

        sprintf(inputs.summary.content, "%s", event->summary);
        inputs.summary.index = strlen(inputs.summary.content);

    } else {
        strcpy(inputs.suffix.content, "AM");
    }

    render_input_fields(modal, &inputs);
    wrefresh(modal);
    refresh_controls(MODAL_WIN);

    int ch = wgetch(modal);
    while (ch != 10 && ch != 27) {
        switch (ch) {
            case KEY_BACKSPACE: {
                delete_byte(&inputs);
                render_input_fields(modal, &inputs);
                break;
            }
            case '\t': {
                if (inputs.all_day) break;

                inputs.active_input = (inputs.active_input + 1) % 4;
                render_input_fields(modal, &inputs);
                break;
            }
            case CTRL('a'): {
                inputs.all_day = !inputs.all_day;
                inputs.active_input = SUMMARY;
                mvwprintw(modal, 3, 3, "        ");
                render_input_fields(modal, &inputs);
                break;
            }
            case 'p': {
                if (inputs.active_input == SUFFIX) {
                    strcpy(inputs.suffix.content, "PM");
                    render_input_fields(modal, &inputs);
                    break;
                }
            }
            case 'a': {
                if (inputs.active_input == SUFFIX) {
                    strcpy(inputs.suffix.content, "AM");
                    render_input_fields(modal, &inputs);
                    break;
                }
            }
            default: {
                set_byte(&inputs, ch);
                render_input_fields(modal, &inputs);
            }
        }

        ch = wgetch(modal);
    }

    struct event new_event = {0};

    if (ch == 10) {
        if (inputs.all_day) {
            new_event.hour = -1;
            new_event.min  = -1;
        } else {
            new_event.hour = atoi(inputs.hour.content);
            new_event.min = atoi(inputs.min.content);
        }

        trim(inputs.summary.content);
        new_event.summary = strdup(inputs.summary.content);
    }

    if (strcmp(inputs.suffix.content, "PM") == 0 && !inputs.all_day) 
        new_event.hour += 12;

    werase(modal);
    wrefresh(modal);
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
        if (inputs->hour.index >= 2 || ch < 48 || ch > 57) return;

        inputs->hour.content[inputs->hour.index] = ch;
        inputs->hour.index++;

    } else if (inputs->active_input == MIN) {
        if (inputs->min.index >= 2 || ch < 48 || ch > 57) return;

        inputs->min.content[inputs->min.index] = ch;
        inputs->min.index++;
    } else if (inputs->active_input == SUMMARY) {
        if (inputs->summary.index >= 1999) return;

        inputs->summary.content[inputs->summary.index] = ch;
        inputs->summary.index++;
    }
}

void delete_byte(Inputs* inputs) {
    switch (inputs->active_input) {
        case HOUR: {
            if (inputs->hour.index > 0) inputs->hour.index--;
            inputs->hour.content[inputs->hour.index] = ' ';
            break;
        }
        case MIN: {
            if (inputs->min.index > 0) inputs->min.index--;
            inputs->min.content[inputs->min.index] = ' ';
            break;
        }
        case SUMMARY: {
            if (inputs->summary.index > 0) inputs->summary.index--;
            inputs->summary.content[inputs->summary.index] = ' ';
            break;
        }
        case SUFFIX: break;
    };
}

void render_input_fields(WINDOW* win, Inputs* inputs) {
    mvwprintw(win, 2, 3, "Time:");
    if (!inputs->all_day) {
        mvwprintw(inputs->hour.win, 0, 0, "%s", inputs->hour.content);
        if (inputs->active_input == HOUR) {
            wbkgd(inputs->hour.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
        } else {
            wbkgd(inputs->hour.win, COLOR_PAIR(INPUT_FIELD_PAIR));
        }

        mvwprintw(win, 3, 5, ":");

        mvwprintw(inputs->min.win, 0, 0, "%s", inputs->min.content);
        if (inputs->active_input == MIN) {
            wbkgd(inputs->min.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
        } else {
            wbkgd(inputs->min.win, COLOR_PAIR(INPUT_FIELD_PAIR));
        }

        mvwprintw(inputs->suffix.win, 0, 0, "%s", inputs->suffix.content);
        if (inputs->active_input == SUFFIX) {
            wbkgd(inputs->suffix.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
        } else {
            wbkgd(inputs->suffix.win, COLOR_PAIR(INPUT_FIELD_PAIR));
        }
    } else {
        werase(inputs->hour.win);
        werase(inputs->min.win);
        werase(inputs->suffix.win);
        mvwprintw(win, 3, 3, "ALL DAY ");
    }

    mvwprintw(win, 6, 3, "Summary (2000 character limit):");
    mvwprintw(inputs->summary.win, 0, 0, "%s", inputs->summary.content);
    if (inputs->active_input == SUMMARY) {
        wbkgd(inputs->summary.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
    } else {
        wbkgd(inputs->summary.win, COLOR_PAIR(INPUT_FIELD_PAIR));
    }

    wrefresh(win);
    wrefresh(inputs->summary.win);
    wrefresh(inputs->hour.win);
    wrefresh(inputs->min.win);
    wrefresh(inputs->suffix.win);
}

void init_inputs(Inputs* inputs, WINDOW* modal, int width) {
    memset(inputs, 0, sizeof(Inputs));
    inputs->active_input = HOUR;
    inputs->all_day = false;

    strcpy(inputs->hour.content, "  ");
    strcpy(inputs->min.content, "  ");

    inputs->hour.win    = derwin(modal, 1, 2, 3, 3);
    inputs->min.win     = derwin(modal, 1, 2, 3, 6);
    inputs->suffix.win  = derwin(modal, 1, 2, 3, 9);
    inputs->summary.win = derwin(modal, 10, width - 6, 7, 3);
}
