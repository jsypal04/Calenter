#include <assert.h>
#include <stdint.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "calenter.h"
#include "calendartxt.h"
#include "config.h"
#include "debug.h"

#define MODAL_WIDTH_BREAKPOINT 85

enum input_tag {
    HOUR,
    MIN,
    SUFFIX,
    E_HOUR,
    E_MIN,
    E_SUFFIX,
    SUMMARY,
    FREQ_SELECT,
    INTERVAL,
    UNTIL
};

typedef struct _input_field {
    WINDOW* win;
    int index;
    enum input_tag tag;
    char content[2048];
} InputField;

typedef struct _input_fields {
    bool           all_day;
    // active_input is an index into the input_cycle
    uint8_t        active_input;
    enum FREQ      freq_select;
    InputField     hour;
    InputField     min;
    InputField     suffix;
    InputField     e_hour;
    InputField     e_min;
    InputField     e_suffix;
    InputField     summary;
    InputField     interval;
    InputField     until;
} Inputs;


void set_byte(Inputs* inputs, char ch);
void delete_byte(InputField* input, char ch, char* def);
void render_input_fields(WINDOW* win, Inputs* inputs, Config config);
void init_inputs(Inputs* inputs, WINDOW* modal, int width);
void render_freq_select_menu(WINDOW* win, Inputs* inputs, int y, int x, bool active);
void render_rrule_params(WINDOW* win, Inputs* inputs, int y, int x, int lines);
void render_input(WINDOW* win, int y, int x, bool active, char* str);
void render_time_input(InputField field, enum input_tag active_input);
void set_input_cycle(uint16_t total_num_inputs);

int set_time_byte(InputField* field, char ch);

uint8_t index_of_input(enum input_tag input, uint16_t total_num_inputs);

char* map_freq(enum FREQ freq);

int height, width;
enum input_tag input_cycle[10];

struct event add_event_modal(
    Window** windows, 
    struct event* event, 
    Config config
) {
    height = 3 * LINES / 4;
    width = COLS / 2;

    uint16_t total_num_inputs = config.send_to_gcal ? 10 : 7;
    set_input_cycle(total_num_inputs);

    if (width < MODAL_WIDTH_BREAKPOINT) {
        width = COLS - 2;
    }

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

        uint8_t index_of_summary = index_of_input(SUMMARY, total_num_inputs);

        if (event->all_day) {
            inputs.all_day = true;
            inputs.active_input = index_of_summary;
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
    } 

    render_input_fields(modal, &inputs, config);
    wrefresh(modal);
    refresh_controls(MODAL_WIN);

    int ch = wgetch(modal);
    while (ch != 10 && ch != 27) {
        switch (ch) {
        // Bindings for the time fields
        case 'p':
            if (inputs.active_input == SUFFIX) {
                strcpy(inputs.suffix.content, "PM");
            } else if (inputs.active_input == E_SUFFIX) {
                strcpy(inputs.e_suffix.content, "PM");
            }

            render_input_fields(modal, &inputs, config);
            goto user_default;

        case 'a':
            if (inputs.active_input == SUFFIX) {
                strcpy(inputs.suffix.content, "AM");
            } else if (inputs.active_input == E_SUFFIX) {
                strcpy(inputs.e_suffix.content, "AM");
            }

            render_input_fields(modal, &inputs, config);
            goto user_default;

        // Bindings for the frequency select dropdown
        case KEY_DOWN:
            if (input_cycle[inputs.active_input] != FREQ_SELECT) break;
            inputs.freq_select = (inputs.freq_select + 1) % 7;
            render_input_fields(modal, &inputs, config);
            break;

        case KEY_UP:
            if (input_cycle[inputs.active_input] != FREQ_SELECT) break;
            inputs.freq_select =
                inputs.freq_select != 0 ?
                inputs.freq_select - 1 : 6;
            render_input_fields(modal, &inputs, config);
            break;

        // General modal bindings
        case '\t': {
            int num_inputs =
                inputs.freq_select != NONE ? total_num_inputs : total_num_inputs - 2;

            if (inputs.all_day) {
                inputs.active_input =
                    inputs.active_input < num_inputs - 1 ?
                    inputs.active_input + 1 : index_of_input(SUMMARY, total_num_inputs);
            } else {
                inputs.active_input = (inputs.active_input + 1) % num_inputs;
            }
            render_input_fields(modal, &inputs, config);
            break;
        }

        case KEY_BTAB: {
            int num_inputs =
                inputs.freq_select != NONE ? total_num_inputs : total_num_inputs - 2;

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
            render_input_fields(modal, &inputs, config);
            break;
        }

        case CTRL('a'):
            inputs.all_day = !inputs.all_day;
            inputs.active_input = index_of_input(SUMMARY, total_num_inputs);
            mvwprintw(modal, 3, 3, "        ");
            render_input_fields(modal, &inputs, config);
            break;

        case KEY_BACKSPACE:
            switch (input_cycle[inputs.active_input]) {
                case HOUR: delete_byte(&inputs.hour, ' ', NULL); break;
                case MIN:  delete_byte(&inputs.min, ' ', NULL); break;
                case E_HOUR:  delete_byte(&inputs.e_hour, ' ', NULL); break;
                case E_MIN:  delete_byte(&inputs.e_min, ' ', NULL); break;
                case SUMMARY: delete_byte(&inputs.summary, ' ', NULL); break;
                case UNTIL: delete_byte(&inputs.until, ' ', "yyyy-mm-dd"); break;
                case INTERVAL: delete_byte(&inputs.interval, '\0', NULL); break;
                default: break;
            }
            render_input_fields(modal, &inputs, config);
            break;

        user_default:
        default:
            set_byte(&inputs, ch);
            render_input_fields(modal, &inputs, config);
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

    struct tm* end = NULL;

    if (config.send_to_gcal) {
        end = malloc(sizeof(struct tm));
        memset(end, 0, sizeof(struct tm));

        end->tm_hour = atoi(inputs.e_hour.content);
        end->tm_min  = atoi(inputs.e_min.content);
        
        if (
            strcmp(inputs.e_suffix.content, "PM") == 0 && 
            end->tm_hour != 12
        ) {
            end->tm_hour += 12;
        } else if (
            strcmp(inputs.e_suffix.content, "AM") == 0 &&
            end->tm_hour == 12
        ) {
            end->tm_hour = 0;
        }
    }
    new_event.end = end;

    return new_event;
}

void set_byte(Inputs* inputs, char ch) {
    switch (input_cycle[inputs->active_input]) {
    case E_HOUR:
        if (set_time_byte(&inputs->e_hour, ch) != 0) return;
        break;

    case HOUR:
        if (set_time_byte(&inputs->hour, ch) != 0) return;
        break;

    case E_MIN:
        if (set_time_byte(&inputs->e_min, ch) != 0) return;
        break;

    case MIN:
        if (set_time_byte(&inputs->min, ch) != 0) return;
        break;

    case SUMMARY:
        if (inputs->summary.index >= 1999 || ch == ',') return;

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

// NOTE: using def to set a default value when the buffer is blank is dangerous.
// The default value is copied with no buffer size check.
void delete_byte(InputField* input, char ch, char* def) {
    if (def != NULL && input->index == 1) {
        strcpy(input->content, def);
        return;
    }
    if (input->index > 0) input->index--;
    input->content[input->index] = ' ';
}

void render_input_fields(WINDOW* win, Inputs* inputs, Config config) {
    // Erase the end time fields
    mvwprintw(win, 2, 25, "         ");
    mvwprintw(win, 3, 25, "         ");

    mvwprintw(win, 2, 3, "Time:");
    if (!inputs->all_day) {
        render_time_input(inputs->hour, inputs->active_input);
        mvwprintw(win, 3, 5, ":");
        render_time_input(inputs->min, inputs->active_input);
        render_time_input(inputs->suffix, inputs->active_input);

        if (config.send_to_gcal) {
            mvwprintw(win, 2, 25, "End Time:");
            render_time_input(inputs->e_hour, inputs->active_input);
            mvwprintw(win, 3, 27, ":");
            render_time_input(inputs->e_min, inputs->active_input);
            render_time_input(inputs->e_suffix, inputs->active_input);
        }
    } else {
        werase(inputs->hour.win);
        werase(inputs->min.win);
        werase(inputs->suffix.win);
        mvwprintw(win, 3, 3, "ALL DAY ");
    }

    mvwprintw(win, 6, 3, "Summary (2000 character limit):");
    mvwprintw(inputs->summary.win, 0, 0, "%s", inputs->summary.content);
    if (input_cycle[inputs->active_input] == SUMMARY) {
        wbkgd(inputs->summary.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
    } else {
        wbkgd(inputs->summary.win, COLOR_PAIR(INPUT_FIELD_PAIR));
    }

    const int RRULE_LENGTH = 58;
    // Clear the entire rrule params section (+2 for margins)
    int lines = ceil((double)RRULE_LENGTH / width);
    for (int l = 0; l < lines * 2; l += 2) {
        for (int i = 1; i < width; i++) {
            mvwprintw(win, 19 + l, i, " ");
        }
    }

    mvwprintw(win, 19, 3, "Repeat");
    render_freq_select_menu(win, inputs, 19, 10, input_cycle[inputs->active_input] == FREQ_SELECT);

    if (inputs->freq_select != NONE)
        render_rrule_params(win, inputs, 19, 21, lines);

    box(win, 0, 0);

    wrefresh(win);
    wrefresh(inputs->summary.win);
    wrefresh(inputs->hour.win);
    wrefresh(inputs->min.win);
    wrefresh(inputs->suffix.win);
}

void init_inputs(Inputs* inputs, WINDOW* modal, int width) {
    memset(inputs, 0, sizeof(Inputs));
    inputs->active_input = 0;
    inputs->all_day = false;

    strcpy(inputs->hour.content, "  ");
    strcpy(inputs->min.content, "  ");
    strcpy(inputs->e_hour.content, "  ");
    strcpy(inputs->e_min.content, "  ");

    strcpy(inputs->suffix.content, "AM");
    strcpy(inputs->e_suffix.content, "AM");

    inputs->hour.win      = derwin(modal, 1, 2, 3, 3);
    inputs->min.win       = derwin(modal, 1, 2, 3, 6);
    inputs->suffix.win    = derwin(modal, 1, 2, 3, 9);
    inputs->e_hour.win    = derwin(modal, 1, 2, 3, 25);
    inputs->e_min.win     = derwin(modal, 1, 2, 3, 28);
    inputs->e_suffix.win  = derwin(modal, 1, 2, 3, 31);
    inputs->summary.win   = derwin(modal, 10, width - 6, 7, 3);
    inputs->freq_select   = NONE;

    inputs->hour.tag        = HOUR;
    inputs->min.tag         = MIN;
    inputs->suffix.tag      = SUFFIX;
    inputs->e_hour.tag      = E_HOUR;
    inputs->e_min.tag       = E_MIN;
    inputs->e_suffix.tag    = E_SUFFIX;
    inputs->summary.tag     = SUMMARY;
    inputs->interval.tag    = INTERVAL;
    inputs->until.tag       = UNTIL;

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

void render_rrule_params(WINDOW* win, Inputs* inputs, int y, int x, int lines) {
    if (lines > 2)
        debug_log("%d lines needed for rrule params. Not splitting accross multiple lines. Please fix.\n", lines);

    int offset = 0;

    mvwprintw(win, y, x, "every");
    offset += strlen("every") + 1;
    render_input(win, y, x + offset,
        input_cycle[inputs->active_input] == INTERVAL, inputs->interval.content);
    offset += strlen(inputs->interval.content) + 2;
    mvwprintw(win, y, x + offset, "%s", map_freq_units(inputs->freq_select));
    offset =
        inputs->freq_select != NONE ?
        offset + strlen(map_freq_units(inputs->freq_select)) :
        offset + 1;

    if (lines == 2) {
        y += 2;
        offset = 0;
    }

    mvwprintw(win, y, x + offset, "until");
    offset += strlen("until") + 1;
    render_input(win, y, x + offset, input_cycle[inputs->active_input] == UNTIL,
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

void render_time_input(InputField field, enum input_tag active_input) {
    mvwprintw(field.win, 0, 0, "%s", field.content);
    if (input_cycle[active_input] == field.tag) {
        wbkgd(field.win, COLOR_PAIR(ACTIVE_INPUT_FIELD_PAIR));
    } else {
        wbkgd(field.win, COLOR_PAIR(INPUT_FIELD_PAIR));
    }
}

int set_time_byte(InputField* field, char ch) {
    if (field->index >= 2 || ch < 48 || ch > 57) return 1;

    field->content[field->index] = ch;
    field->index++;
    return 0;
}

void set_input_cycle(uint16_t total_num_inputs) {
    input_cycle[0] = HOUR;
    input_cycle[1] = MIN;
    input_cycle[2] = SUFFIX;
    
    uint8_t i = 3;
    if (total_num_inputs == 10) {
        input_cycle[3] = E_HOUR;
        input_cycle[4] = E_MIN;
        input_cycle[5] = E_SUFFIX;
        i = 6;
    }

    input_cycle[i]     = SUMMARY;
    input_cycle[i + 1] = FREQ_SELECT;
    input_cycle[i + 2] = INTERVAL;
    input_cycle[i + 3] = UNTIL;
}

uint8_t index_of_input(enum input_tag input, uint16_t total_num_inputs) {
    uint8_t index;
    for (int i = 0; i < total_num_inputs; i++) {
        if (input_cycle[i] != input) continue;
        index = i;
        break;
    }
    return index;
}
