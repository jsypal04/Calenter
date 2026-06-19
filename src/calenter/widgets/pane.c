#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "pane.h"
#include "../calenter.h"
#include "../utils/ncurses-utils.h"
#include "../layout/layout.h"
#include "../array/array.h"


UIPane* new_ui_pane(UILayout* parent_layout, int id, char* title) {
    UIPane* pane = malloc(sizeof(UIPane));
    bzero(pane, sizeof(UIPane));

    UIObject* object = new_ui_object(id);
    register_obj(parent_layout, object);

    pane->win = newwin(
        get_height(parent_layout, id),
        get_width(parent_layout, id),
        get_starty(parent_layout, id),
        get_startx(parent_layout, id)
    );
    pane->layout = new_layout(
        get_height(parent_layout, id),
        get_width(parent_layout, id),
        ROW
    );
    pane->title  = strdup(title);
    pane->is_active = false;

    object->componant = PANE;
    object->data.pane = pane;

    return pane;
}

void free_ui_pane(UIPane *pane) {
    free(pane->title);
    delwin(pane->win);

    pane->title = NULL;
    pane->win   = NULL;

    free_layout(pane->layout);

    free(pane);
    pane = NULL;
}

void render_ui_pane(UIObject *object) {
    assert(object->componant == PANE);
    assert(object->data.pane->layout != NULL);
    assert(object->data.pane->layout->layout_objs != NULL);
    assert(array_type(object->data.pane->layout->layout_objs) == UI_OBJECT);
    assert(strlen(object->data.pane->title) < 256);

    UIPane* pane = object->data.pane;

    wattron(pane->win, A_BOLD);
    if (pane->is_active) {
        wattron(pane->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
        box(pane->win, 0, 0);

        if (pane->title != NULL)
            mvwprintw(pane->win, 0, 1, " %s ", pane->title);

        wattroff(pane->win, COLOR_PAIR(ACTIVE_COLOR_PAIR));
    } else {
        box(pane->win, 0, 0);

        if (pane->title != NULL)
            mvwprintw(pane->win, 0, 1, " %s ", pane->title);
    }
    wattroff(pane->win, A_BOLD);

    for (int i = 0; i < array_len(pane->layout->layout_objs); i++) {
        UIObject* obj = get_UIObject(pane->layout->layout_objs, i);
        obj->render(obj);
    }
}
