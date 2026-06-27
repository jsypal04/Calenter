#include <assert.h>
#include <stdlib.h>
#include <strings.h>
#include "layout.h"
#include "../widgets/pane.h"
#include "../array/array.h"
#include "../utils/debug.h"

UILayout* new_layout(int height, int width, LayoutType type) {
    UILayout* layout = malloc(sizeof(UILayout));
    bzero(layout, sizeof(UILayout));

    layout->height = height;
    layout->width  = width;
    layout->layout_objs = new_array(10, UI_OBJECT);
    layout->layout_type = type;

    return layout;
}

void free_layout(UILayout* layout) {
    LOG_FUNC("Running free_layout");

    free_array(layout->layout_objs);
    free(layout);
    layout = NULL;
}

UIFloat sum(UIFloat f1, UIFloat f2) {
    assert(f1.unit == f2.unit);
    UIFloat result = {0};
    result.value = f1.value + f2.value;
    result.unit = f1.unit;
    return result;
}

void render(UILayout* layout) {
    int num_objects = array_len(layout->layout_objs);

    for (int i = 0; i < num_objects; i++) {
        UIObject* object = get_UIObject(layout->layout_objs, i);
        object->render(object);
    }
}

void resize_layout(UILayout* layout, int height, int width) {
    layout->height = height;
    layout->width  = width;
}

UIObject* new_ui_object(int id) {
    UIObject* layout_obj = malloc(sizeof(UIObject));
    bzero(layout_obj, sizeof(UIObject));

    layout_obj->id = id;

    return layout_obj;
}

void free_ui_object(UIObject* object) {
    LOG_FUNC("Running free_ui_object");

    debug_log("object->data.pane = %x\n");
    if (object->data.pane != NULL)
        free_ui_pane(object->data.pane);

    free(object);
    object = NULL;
}

UIFloat new_ui_float(float value, UIUnit unit) {
    UIFloat f = {0};
    f.value = value;
    f.unit = unit;
    return f;
}

void register_ui_pane(UILayout* layout, UIPane* pane, int id) {
    // Enforce ID uniqueness (should replace the asserts eventually)
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        UIObject* o = get_UIObject(layout->layout_objs, i);
        assert(o->id != id);
    }
    UIObject* object = new_ui_object(id);

    object->componant = PANE;
    object->data.pane = pane;

    object->render = render_ui_pane;
    object->resize = resize_ui_pane;

    append_UIObject(layout->layout_objs, object);
}

void set_row_layout(UILayout* layout) {
    LOG_FUNC("Running set_row_layout");

    int num_objects = array_len(layout->layout_objs);

    float width_percent = 100.0 / (float)num_objects;

    UIFloat  width = new_ui_float(width_percent, PERCENT);
    UIFloat height = new_ui_float(100.0, PERCENT);
    UIFloat startx = new_ui_float(0.0, PERCENT);
    UIFloat starty = new_ui_float(0.0, PERCENT);

    for (int i = 0; i < num_objects; i++) {
        UIObject* obj = get_UIObject(layout->layout_objs, i);

        obj->width = width;
        obj->height = height;
        obj->startx = startx;
        obj->starty = starty;

        obj->resize(layout, obj);

        startx = sum(width, startx);
    }
}

void set_layout(UILayout* layout) {
    LOG_FUNC("Running set_layout");

    switch (layout->layout_type) {
        case ROW:
        set_row_layout(layout);
    }
}

int get_value(
    UILayout* layout,
    int id,
    UIFloat (*extractor)(UIObject*),
    int   (*parent_extractor)(UILayout*)
) {
    bool not_found = true;
    UIObject* obj;
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        obj = get_UIObject(layout->layout_objs, i);

        if (obj->id == id) {
            not_found = false;
            break;
        }
    }

    if (not_found) return -1;

    UIFloat ui_value = extractor(obj);

    if (ui_value.unit == CELLS) return ui_value.value;

    assert(ui_value.unit == PERCENT);

    int parent_value = parent_extractor(layout);
    int value = (int)((ui_value.value / 100.0) * (float)parent_value);
    return value;
}

UIFloat width_extractor(UIObject* obj) {
    return obj->width;
}

UIFloat height_extractor(UIObject* obj) {
    return obj->height;
}

UIFloat startx_extractor(UIObject* obj) {
    return obj->startx;
}

UIFloat starty_extractor(UIObject* obj) {
    return obj->starty;
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
