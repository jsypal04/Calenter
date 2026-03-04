#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "calenter.h"

extern Window* windows[NUM_WINDOWS];

void refresh_controls(int win_id);

Window* create_win(int id, char* title, int height, int width, int startx, int starty) {
    Window* window = malloc(sizeof(Window));

    window->id = id;
    window->title = title == NULL ? NULL : strdup(title);
    window->width = width;
    window->height = height;
    window->widgets = NULL;
    window->win = newwin(height, width, starty, startx);

    keypad(window->win, true);

    refresh_win(window, false);

    return window;
}

void free_win(Window* window) {
    delwin(window->win);
    free(window->title);
    free(window->widgets);
    free(window);

    window = NULL;
}

void refresh_win(Window* window, bool active) {
    if (window->title == NULL) {
        wrefresh(window->win);
        return;
    }

    wattron(window->win, A_BOLD);
    if (active) {
        wattron(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
        wattroff(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    } else {
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
    }
    wattroff(window->win, A_BOLD);
    wrefresh(window->win);
}

void set_active_window(Window** active_win, Window* window) {
    Window* current_active_win = *active_win;
    if (current_active_win != NULL) {
        refresh_win(current_active_win, false);
    }

    *active_win = window;
    wattron(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    refresh_win(window, true);

    refresh_controls(window->id);
}

void refresh_controls(int win_id) {
    char common_ctrls[256] = "h,j,k,l  Nav | q  Quit | s  Sync";

    char controls_str[4096] = "\0";
    strcpy(controls_str, common_ctrls);

    switch (win_id) {
        case SCHEDULE_WIN:
            strcpy(controls_str + strlen(common_ctrls), " | d  Delete | ENTER  Add/Edit");
            break;

        case CALENDAR_WIN:
            strcpy(controls_str + strlen(common_ctrls), " | ENTER  Go to Day");
            break;
    }

    int x = (windows[CONTROLS_WIN]->width - strlen(controls_str)) / 2;

    werase(windows[CONTROLS_WIN]->win);
    wattron(windows[CONTROLS_WIN]->win, COLOR_PAIR(CONTROLS_COLOR_PAIR));
    mvwprintw(windows[CONTROLS_WIN]->win, 1, x, "%s", controls_str);
    wattroff(windows[CONTROLS_WIN]->win, COLOR_PAIR(CONTROLS_COLOR_PAIR));
    refresh_win(windows[CONTROLS_WIN], false);
}
