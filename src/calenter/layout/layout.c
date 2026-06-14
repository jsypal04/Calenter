#include <assert.h>
#include <stdlib.h>
#include <strings.h>
#include "layout.h"
#include "../array/array.h"

UILayout* init_layout(int height, int width) {
    UILayout* layout = malloc(sizeof(UILayout));
    bzero(layout, sizeof(UILayout));

    layout->height = height;
    layout->width  = width;
    layout->layout_objs = new_array(10, UI_LAYOUT_OBJ);

    return layout;
}

void free_layout(UILayout* layout) {
    free_array(layout->layout_objs);
    free(layout);
}

void resize_layout(UILayout* layout, int height, int width) {
    layout->height = height;
    layout->width  = width;
}

UILayoutObj init_layout_obj(
    int id, float height, float width, float starty, float startx, UIUnit unit
) {
    UILayoutObj layout_obj = {0};
    layout_obj.id       = id;
    layout_obj.r_height = height;
    layout_obj.r_width  = width;
    layout_obj.r_starty = starty;
    layout_obj.r_startx = startx;
    layout_obj.unit     = unit;
    layout_obj.layout   = NULL;

    return layout_obj;
}

void register_obj(UILayout* layout, UILayoutObj obj) {
    // Enforce ID uniqueness (should replace the asserts eventually)
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        UILayoutObj o = get_UILayoutObj(layout->layout_objs, i);
        assert(o.id != obj.id);
    }

    append_UILayoutObj(layout->layout_objs, obj);
}

int get_value(
    UILayout* layout,
    int id,
    float (*extractor)(UILayoutObj),
    int   (*parent_extractor)(UILayout*)
) {
    bool not_found = true;
    UILayoutObj obj;
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        obj = get_UILayoutObj(layout->layout_objs, i);

        if (obj.id == id) {
            not_found = false;
            break;
        }
    }

    if (not_found) return -1;
    if (obj.unit == CELLS) return extractor(obj);

    assert(obj.unit == PERCENT);

    int width = (int)((extractor(obj) / 100.0) * (float)parent_extractor(layout));
    return width;
}

float width_extractor(UILayoutObj obj) {
    return obj.r_width;
}

float height_extractor(UILayoutObj obj) {
    return obj.r_height;
}

float startx_extractor(UILayoutObj obj) {
    return obj.r_startx;
}

float starty_extractor(UILayoutObj obj) {
    return obj.r_starty;
}

int parent_width_extractor(UILayout* layout) {
    return layout->width;
}

int parent_height_extractor(UILayout* layout) {
    return layout->height;
}

int get_width(UILayout* layout, int id) {
    int val = get_value(layout, id, width_extractor, parent_width_extractor);
    return val;
}

int get_height(UILayout* layout, int id) {
    int val = get_value(layout, id, height_extractor, parent_height_extractor);
    return val;
}

int get_startx(UILayout* layout, int id) {
    int val = get_value(layout, id, startx_extractor, parent_width_extractor);
    return val;
}

int get_starty(UILayout* layout, int id) {
    int val = get_value(layout, id, starty_extractor, parent_height_extractor);
    return val;
}
