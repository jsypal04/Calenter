#include <ncurses.h>

#include "ncurses-utils.h"

void setup_ncurses() {
    initscr();
    set_escdelay(25);
    curs_set(0);
    clear();
    noecho();
    cbreak();
    start_color();

    init_color(DARK_GREY, 251, 251, 251);

    init_pair(ACTIVE_COLOR_PAIR, COLOR_GREEN, COLOR_BLACK);
    init_pair(INACTIVE_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(INPUT_FIELD_PAIR, COLOR_WHITE, DARK_GREY);
    init_pair(ACTIVE_INPUT_FIELD_PAIR, COLOR_WHITE, 8);
    init_pair(CONTROLS_COLOR_PAIR, COLOR_BLUE, COLOR_BLACK);
}
