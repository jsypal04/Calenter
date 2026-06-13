#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "calenter.h"

#define NUM_INPUTS 7

enum input_tag {
    HOUR,
    MIN,
    SUFFIX,
    SUMMARY,
    FREQ_SELECT,
    INTERVAL,
    UNTIL
};

typedef struct _input_field {
    WINDOW* win;
    int index;
    char content[2048];
} InputField;

typedef struct _input_fields {
    bool           all_day;
    enum input_tag active_input;
    enum FREQ      freq_select;
    InputField     hour;
    InputField     min;
    InputField     suffix;
    InputField     summary;
    InputField     interval;
    InputField     until;
} Inputs;


void set_byte(Inputs* inputs, char ch);
void delete_byte(Inputs* inputs);
void render_input_fields(WINDOW* win, Inputs* inputs);
void init_inputs(Inputs* inputs, WINDOW* modal, int width);
void render_freq_select_menu(WINDOW* win, Inputs* inputs, int y, int x, bool active);
void render_rrule_params(WINDOW* win, Inputs* inputs, int y, int x);
void render_input(WINDOW* win, int y, int x, bool active, char* str);

char* map_freq(enum FREQ freq);

int height, width;

struct event add_event_modal(Window** windows, struct event* event) {
    height = 3 * LINES / 4;
    width = COLS / 2;

    WINDOW* modal = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    keypad(modal, true);
    box(modal, 0, 0);

    Inputs inputs;
    init_inputs(&inputs, modal, width);

    if (event != NULL) {
        assert(strlen(event->summary) < 2000);

        if (event->datetime.tm_hour >= 12) {
            strcpy(inputs.suffix.content, "PM");
        } else {
            strcpy(inputs.suffix.content, "AM");
        }

        int hour = event->datetime.tm_hour != 12 ? event->datetime.tm_hour % 12 : 12;

        char* hour_fmt_str = (hour < 10) ? "0%d" : "%d";
        char* min_fmt_str = (event->datetime.tm_min < 10) ? "0%d" : "%d";

        if (event->all_day) {
            inputs.all_day = true;
            inputs.active_input = SUMMARY;
        } else {
            sprintf(inputs.hour.content, hour_fmt_str, hour);
            sprintf(inputs.min.content, min_fmt_str, event->datetime.tm_min);

            inputs.hour.index = strlen(inputs.hour.content);
            inputs.min.index = strlen(inputs.min.content);
        }

        sprintf(inputs.summary.content, "%s", event->summary);
        inputs.summary.index = strlen(inputs.summary.content);

        inputs.freq_select = event->rrule.freq;
        // TODO: populate interval
    } else {
        strcpy(inputs.suffix.content, "AM");
    }

    render_input_fields(modal, &inputs);
    wrefresh(modal);
    refresh_controls(MODAL_WIN);

    int ch = wgetch(modal);
    while (ch != 10 && ch != 27) {
        switch (ch) {
        // Bindings for the time fields
        case 'p':
            if (inputs.active_input != SUFFIX)
                goto user_default;

            strcpy(inputs.suffix.content, "PM");
            render_input_fields(modal, &inputs);
            goto user_default;

        case 'a':
            if (inputs.active_input != SUFFIX)
                goto user_default;

            strcpy(inputs.suffix.content, "AM");
            render_input_fields(modal, &inputs);
            goto user_default;

        // Bindings for the frequency select dropdown
        case KEY_DOWN:
            if (inputs.active_input != FREQ_SELECT) break;
            inputs.freq_select = (inputs.freq_select + 1) % 7;
            render_input_fields(modal, &inputs);
            break;

        case KEY_UP:
            if (inputs.active_input != FREQ_SELECT) break;
            inputs.freq_select =
                inputs.freq_select != 0 ?
                inputs.freq_select - 1 : 6;
            render_input_fields(modal, &inputs);
            break;

        // General modal bindings
        case '\t': {
            int num_inputs =
                inputs.freq_select != NONE ? NUM_INPUTS : NUM_INPUTS - 2;

            if (inputs.all_day) {
                inputs.active_input =
                    inputs.active_input < num_inputs - 1 ?
                    inputs.active_input + 1 : 3;
            } else {
                inputs.active_input = (inputs.active_input + 1) % num_inputs;
            }
            render_input_fields(modal, &inputs);
            break;
        }

        case KEY_BTAB: {
            int num_inputs =
                inputs.freq_select != NONE ? NUM_INPUTS : NUM_INPUTS - 2;

            if (inputs.all_day) {
                inputs.active_input =
                    inputs.active_input > 3 ?
                    inputs.active_input - 1 :
                    num_inputs - 1;
            } else {
                inputs.active_input =
                    inputs.active_input > 0 ?
                    inputs.active_input - 1 :
                    num_inputs - 1;
            }
            render_input_fields(modal, &inputs);
            break;
        }

        case CTRL('a'):
            inputs.all_day = !inputs.all_day;
            inputs.active_input = SUMMARY;
            mvwprintw(modal, 3, 3, "        ");
            render_input_fields(modal, &inputs);
            break;

        case KEY_BACKSPACE:
            delete_byte(&inputs);
            render_input_fields(modal, &inputs);
            break;

        user_default:
        default:
            set_byte(&inputs, ch);
            render_input_fields(modal, &inputs);
        }

        ch = wgetch(modal);
    }

    struct event new_event = {0};

    if (ch == 10) {
        if (!inputs.all_day) {
            new_event.datetime.tm_hour = atoi(inputs.hour.content);
            new_event.datetime.tm_min = atoi(inputs.min.content);

            if (
                strcmp(inputs.suffix.content, "PM") == 0 &&
                new_event.datetime.tm_hour != 12
            ) {
                new_event.datetime.tm_hour += 12;
            } else if (
                strcmp(inputs.suffix.content, "AM") == 0 &&
                new_event.datetime.tm_hour == 12
            ) {
                new_event.datetime.tm_hour = 0;
            }
        } else {
            new_event.all_day = true;
        }

        trim(inputs.summary.content);
        new_event.summary = strdup(inputs.summary.content);

        trim(inputs.until.content);
        if (inputs.freq_select != NONE && verify_date(inputs.until.content)) {
            char year[5]  = "\0";
            char month[3] = "\0";
            char day[3]   = "\0";

            strncpy(year,  inputs.until.content,     4);
            strncpy(month, inputs.until.content + 5, 2);
            strncpy(day,   inputs.until.content + 8, 2);

            new_event.rrule.until.tm_year = atoi(year) - 1900;
            new_event.rrule.until.tm_mon  = atoi(month) - 1;
            new_event.rrule.until.tm_mday = atoi(day);

            mktime(&new_event.rrule.until);

            new_event.rrule.freq = inputs.freq_select;
            new_event.rrule.interval = atoi(inputs.interval.content);
        }
    }

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
    switch (inputs->active_input) {
    case HOUR:
        if (inputs->hour.index >= 2 || ch < 48 || ch > 57) return;

        inputs->hour.content[inputs->hour.index] = ch;
        inputs->hour.index++;
        break;

    case MIN:
        if (inputs->min.index >= 2 || ch < 48 || ch > 57) return;

        inputs->min.content[inputs->min.index] = ch;
        inputs->min.index++;
        break;

    case SUMMARY:
        if (inputs->summary.index >= 1999) return;

        inputs->summary.content[inputs->summary.index] = ch;
        inputs->summary.index++;
        break;

    case INTERVAL:
        if (inputs->interval.index >= 3 || ch < 48 || ch > 57) return;

        inputs->interval.content[inputs->interval.index] = ch;
        inputs->interval.index++;
        break;

    case UNTIL:
        if (
            inputs->until.index >= 10 ||
            ((ch < 48 || ch > 57) && ch != '-')
        ) return;

        inputs->until.content[inputs->until.index] = ch;
        inputs->until.index++;

        if (inputs->until.index == 1)
           strcpy(inputs->until.content + 1, "         ");
        break;

    default: return;
    }
}

void delete_byte(Inputs* inputs) {
    switch (inputs->active_input) {
    case HOUR:
        if (inputs->hour.index > 0) inputs->hour.index--;
        inputs->hour.content[inputs->hour.index] = ' ';
        break;

    case MIN:
        if (inputs->min.index > 0) inputs->min.index--;
        inputs->min.content[inputs->min.index] = ' ';
        break;

    case SUMMARY:
        if (inputs->summary.index > 0) inputs->summary.index--;
        inputs->summary.content[inputs->summary.index] = ' ';
        break;

    case INTERVAL:
        if (inputs->interval.index > 0) inputs->interval.index--;
        inputs->interval.content[inputs->interval.index] = '\0';
        break;

    case UNTIL:
        if (inputs->until.index > 0) {
            inputs->until.index--;
            inputs->until.content[inputs->until.index] = ' ';
        }

        if (inputs->until.index == 0)
            strcpy(inputs->until.content, "yyyy-mm-dd");

        break;

    default: break;
    }
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

    mvwprintw(win, 19, 3, "Repeat");
    render_freq_select_menu(win, inputs, 19, 10, inputs->active_input == FREQ_SELECT);

    for (int i = 21; i < width; i++) {
        mvwprintw(win, 19, i, " ");
    }

    if (inputs->freq_select != NONE)
        render_rrule_params(win, inputs, 19, 21);

    box(win, 0, 0);

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
    inputs->freq_select = NONE;

    inputs->interval.win = NULL;
    strcpy(inputs->interval.content, "1");
    inputs->interval.index = strlen(inputs->interval.content);

    inputs->until.win = NULL;
    strcpy(inputs->until.content, "yyyy-mm-dd");
}

char* map_freq(enum FREQ freq) {
    switch (freq) {
    case NONE:     return "never   ";
    case YEARLY:   return "yearly  ";
    case MONTHLY:  return "monthly ";
    case WEEKLY:   return "weekly  ";
    case DAILY:    return "daily   ";
    case HOURLY:   return "hourly  ";
    case MINUTELY: return "minutely";
    default:       return "        ";
    }
}

char* map_freq_units(enum FREQ freq) {
    switch (freq) {
    case NONE:     return "         ";
    case YEARLY:   return " years ";
    case MONTHLY:  return " months ";
    case WEEKLY:   return " weeks ";
    case DAILY:    return " days ";
    case HOURLY:   return " hours ";
    case MINUTELY: return " minutes ";
    default:       return "         ";
    }
}

void render_freq_select_menu(WINDOW* win, Inputs* inputs, int y, int x, bool active) {
    if (!active)
        render_input(win, y, x, active, map_freq(inputs->freq_select));

    for (int i = 0; i < 7; i++) {
        if (active) {
            render_input(win, y + i, x, inputs->freq_select == i, map_freq(i));
        } else if (i > 0) {
            mvwprintw(win, y + i, x, " %s ", map_freq(-1));
        }
    }
}

void render_rrule_params(WINDOW* win, Inputs* inputs, int y, int x) {
    int offset = 0;

    mvwprintw(win, y, x, "every");
    offset += strlen("every") + 1;
    render_input(win, y, x + offset,
        inputs->active_input == INTERVAL, inputs->interval.content);
    offset += strlen(inputs->interval.content) + 2;
    mvwprintw(win, y, x + offset, "%s", map_freq_units(inputs->freq_select));
    offset =
        inputs->freq_select != NONE ?
        offset + strlen(map_freq_units(inputs->freq_select)) :
        offset + 1;
    mvwprintw(win, y, x + offset, "until");
    offset += strlen("until") + 1;
    render_input(win, y, x + offset, inputs->active_input == UNTIL,
        inputs->until.content);
}

void render_input(WINDOW* win, int y, int x, bool active, char* str) {
    if (active) {
        wattron(win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
    } else {
        wattron(win, COLOR_PAIR(INPUT_FIELD_PAIR));
    }

    mvwprintw(win, y, x, " %s ", str);

    if (active) {
        wattroff(win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
    } else {
        wattroff(win, COLOR_PAIR(INPUT_FIELD_PAIR));
    }
}
