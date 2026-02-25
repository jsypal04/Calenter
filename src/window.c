#include <stdlib.h>
#include <string.h>
#include "calenter.h"


Window* create_win(int id, char* title, int height, int width, int startx, int starty) {
    Window* window = malloc(sizeof(Window));

    window->id = id;
    window->title = strdup(title);
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
    if (active) {
        wattron(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
        wattroff(window->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    } else {
        box(window->win, 0, 0);
        mvwprintw(window->win, 0, 1, " %s ", window->title);
    }
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
}
