#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "pane.h"
#include "../calenter.h"
#include "../utils/ncurses-utils.h"
#include "../utils/debug.h"
#include "../layout/layout.h"
#include "../array/array.h"

extern UIPane* active_pane;

UIPane* new_ui_pane(UILayout* parent_layout, char* title) {
    LOG_FUNC("Running new_ui_pane");

    UIPane* pane = malloc(sizeof(UIPane));
    bzero(pane, sizeof(UIPane));

    debug_log("pane = %x\n", pane);

    if (title != NULL)
        pane->title = strdup(title);

    pane->is_active = false;

    return pane;
}

void free_ui_pane(UIPane *pane) {
    LOG_FUNC("Running free_ui_pane");

    if (pane->title != NULL)
        free(pane->title);

    if (pane->win != NULL)
        delwin(pane->win);

    pane->title = NULL;
    pane->win   = NULL;

    if (pane->layout != NULL)
        free_layout(pane->layout);

    free(pane);
    pane = NULL;
}

void render_ui_pane(UIObject *object) {
    LOG_FUNC("Running render_ui_pane");

    assert(object->componant == PANE);
    assert(object->data.pane->layout != NULL);
    assert(object->data.pane->layout->layout_objs != NULL);
    assert(array_type(object->data.pane->layout->layout_objs) == UI_OBJECT);
    if (object->data.pane->title != NULL)
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

    render(pane->layout);

    int ret = wrefresh(pane->win);

    if (ret != 0)
        debug_log("wrefresh failed for pane %d\n", object->id);
}

void resize_ui_pane(UILayout* parent_layout, UIObject* object) {
    LOG_FUNC("Running resize_ui_pane");

    assert(object->componant == PANE);

    UIPane* pane = object->data.pane;

    int height = get_height(parent_layout, object->id);
    int width  = get_width(parent_layout, object->id);
    int starty = get_starty(parent_layout, object->id);
    int startx = get_startx(parent_layout, object->id);

    debug_log("height = %d\n", height);
    debug_log("width = %d\n", width);
    debug_log("starty = %d\n", starty);
    debug_log("startx = %d\n", startx);

    if (pane->win == NULL) {
        pane->win = newwin(height, width, starty, startx);
    } else {
        wresize(pane->win, height, width);
        wmove(pane->win, starty, startx);
    }

    if (pane->layout != NULL) {
        pane->layout->height = height;
        pane->layout->width  = width;
    } else {
        // TODO: the layout type needs to be parameterized
        pane->layout = new_layout(height, width, ROW);
    }

    assert(pane->layout->layout_objs != NULL);
    assert(array_type(object->data.pane->layout->layout_objs) == UI_OBJECT);

    int length = array_len(pane->layout->layout_objs);
    for (int i = 0; i < length; i++) {
        UIObject* o = get_UIObject(pane->layout->layout_objs, i);
        o->resize(pane->layout, o);
    }
}


int set_active_pane(UILayout* layout, int id) {
    assert(layout->layout_objs != NULL);
    assert(array_type(layout->layout_objs) == UI_OBJECT);

    int num_objects = array_len(layout->layout_objs);

    UIObject* obj = NULL;
    for (int i = 0; i < num_objects; i++) {
        obj = get_UIObject(layout->layout_objs, i);
        if (obj->id == id) break;
        obj = NULL;
    }

    if (obj == NULL || obj->componant != PANE) return -1;

    obj->data.pane->is_active = true;
    active_pane = obj->data.pane;

    return 0;
}
