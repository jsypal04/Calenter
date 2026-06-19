#include <assert.h>
#include <stdlib.h>
#include <strings.h>
#include "layout.h"
#include "../array/array.h"

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
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
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

    layout_obj->id       = id;

    return layout_obj;
}

void free_ui_object(UIObject* object) {
    free(object);
    object = NULL;
}

UIFloat new_ui_float(float value, UIUnit unit) {
    UIFloat f = {0};
    f.value = value;
    f.unit = unit;
    return f;
}

void add_object_to_row(UILayout* layout, UIObject* obj) {
    assert(array_type(layout->layout_objs) == UI_OBJECT);

    int num_objects = array_len(layout->layout_objs);
    int capacity    = array_cap(layout->layout_objs);

    if (num_objects == 0) {
        obj->startx = new_ui_float(  0.0, PERCENT);
        obj->starty = new_ui_float(  0.0, PERCENT);
        obj->width  = new_ui_float(100.0, PERCENT);
        obj->height = new_ui_float(100.0, PERCENT);
    }

    UIObject* prev_obj = get_UIObject(layout->layout_objs, num_objects - 1);

    float new_width = 100.0 * (((float)num_objects + 1.0) / (float)layout->width);

    obj->startx = sum(prev_obj->startx, prev_obj->width);
    obj->starty = prev_obj->starty;
    obj->width  = new_ui_float(new_width, PERCENT);
    obj->height = prev_obj->height;

    Array* updated_objects = new_array(capacity, UI_OBJECT);
    for (int i = 0; i < num_objects; i++) {
        UIObject* o = get_UIObject(layout->layout_objs, i);
        o->width = new_ui_float(new_width, PERCENT);
        append_UIObject(updated_objects, o);
    }
    free_array(layout->layout_objs);
    layout->layout_objs = updated_objects;
    append_UIObject(layout->layout_objs, obj);
}

void register_obj(UILayout* layout, UIObject* obj) {
    // Enforce ID uniqueness (should replace the asserts eventually)
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        UIObject* o = get_UIObject(layout->layout_objs, i);
        assert(o->id != obj->id);
    }

    switch (layout->layout_type) {
        case ROW:
        add_object_to_row(layout, obj);
        break;
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
