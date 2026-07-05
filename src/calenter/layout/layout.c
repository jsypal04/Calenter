#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "layout.h"
#include "../widgets/text.h"
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

UIFloat product(UIFloat f1, UIFloat f2) {
    assert(f1.unit == f2.unit);
    UIFloat result = {0};
    result.value = f1.value * f2.value;
    result.unit = f1.unit;
    return result;
}

void render(UILayout* layout, UIPane* pane) {
    int num_objects = array_len(layout->layout_objs);

    for (int i = 0; i < num_objects; i++) {
        UIObject* object = get_UIObject(layout->layout_objs, i);
        switch (object->componant) {
            case PANE:
            render_ui_pane(object);
            break;

            case TEXT:
            render_ui_text(pane, object);
            break;
        }
    }
}

UIObject* new_ui_object(int id, GridParams* params) {
    UIObject* layout_obj = malloc(sizeof(UIObject));
    bzero(layout_obj, sizeof(UIObject));

    layout_obj->id = id;

    if (params == NULL) return layout_obj;

    GridParams owned_params = {0};
    owned_params.col = params->col;
    owned_params.row = params->row;
    owned_params.colspan = params->colspan;
    owned_params.rowspan = params->rowspan;

    layout_obj->grid_params = owned_params;

    return layout_obj;
}

void free_ui_object(UIObject* object) {
    LOG_FUNC("Running free_ui_object");

    debug_log("object->data.pane = %x\n");
    switch (object->componant) {
        case PANE:
        if (object->data.pane != NULL)
            free_ui_pane(object->data.pane);
        break;

        case TEXT:
        if (object->data.text != NULL)
            free_ui_text(object->data.text);
        break;
    }

    free(object);
    object = NULL;
}

GridParams new_grid_params(int col, int row, int colspan, int rowspan) {
    GridParams params = {0};
    params.col = col;
    params.row = row;
    params.colspan = colspan;
    params.rowspan = rowspan;
    return params;
}

UIFloat new_ui_float(float value, UIUnit unit) {
    UIFloat f = {0};
    f.value = value;
    f.unit = unit;
    return f;
}

void enforce_id_uniqueness(UILayout* layout, int id) {
    for (int i = 0; i < array_len(layout->layout_objs); i++) {
        UIObject* o = get_UIObject(layout->layout_objs, i);
        assert(o->id != id);
    }
}

void register_ui_pane(UILayout* layout, UIPane* pane, int id, GridParams* params) {
    if (layout->layout_type == GRID) {
        assert(params != NULL);
    }

    enforce_id_uniqueness(layout, id);

    UIObject* object = new_ui_object(id, params);

    object->componant = PANE;
    object->data.pane = pane;

    object->resize = resize_ui_pane;

    append_UIObject(layout->layout_objs, object);
}

void register_ui_text(UILayout* layout, UIText* text, int id, GridParams* params) {
    assert(layout != NULL);
    if (layout->layout_type == GRID) {
        assert(params != NULL);
    }
    enforce_id_uniqueness(layout, id);

    UIObject* object = new_ui_object(id, params);

    object->componant = TEXT;
    object->data.text = text;

    object->resize = resize_ui_text;

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

void set_stack_layout(UILayout* layout) {
   LOG_FUNC("Running set_stack_layout");

   int num_objects = array_len(layout->layout_objs);

   float height_percent = 100.0 / (float)num_objects;

   UIFloat  width = new_ui_float(100.0, PERCENT);
   UIFloat height = new_ui_float(height_percent, PERCENT);
   UIFloat startx = new_ui_float(0.0, PERCENT);
   UIFloat starty = new_ui_float(0.0, PERCENT);

   for (int i = 0; i < num_objects; i++) {
       UIObject* obj = get_UIObject(layout->layout_objs, i);

       obj->width = width;
       obj->height = height;
       obj->startx = startx;
       obj->starty = starty;

       obj->resize(layout, obj);

       starty = sum(height, starty);
   }

}

/**
 * Ensures that the grid params do not result in any widgets overlapping
 * each other.
 *
 * TODO: Need tests for this function
 */
bool validate_grid_layout(UILayout* layout) {
    assert(layout->layout_type == GRID);

    int num_objects = array_len(layout->layout_objs);

    for (int i = 0; i < num_objects; i++) {
        for (int j = i + 1; j < num_objects; j++) {
            UIObject* obj1 = get_UIObject(layout->layout_objs, i);
            UIObject* obj2 = get_UIObject(layout->layout_objs, j);

            if (
                obj1->grid_params.row == obj2->grid_params.row &&
                obj1->grid_params.col == obj2->grid_params.col
            ) {
                return false;
            }

            // I think the - 1 is necessary
            int obj_1_end_row = obj1->grid_params.row + obj1->grid_params.rowspan - 1;
            int obj_1_end_col = obj1->grid_params.col + obj1->grid_params.colspan - 1;
            int obj_2_end_row = obj2->grid_params.row + obj2->grid_params.rowspan - 1;
            int obj_2_end_col = obj2->grid_params.col + obj2->grid_params.colspan - 1;

            for (int r = obj1->grid_params.row; r < obj_1_end_row; r++) {
                for (int c = obj1->grid_params.col; c < obj_1_end_col; c++) {
                    if (
                        r >= obj2->grid_params.row && r <= obj_2_end_row &&
                        c >= obj2->grid_params.col && c <= obj_2_end_col
                    ) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void set_grid_layout(UILayout* layout) {
    LOG_FUNC("Running set_grid_layout");

    assert(layout->layout_objs != NULL);
    assert(validate_grid_layout(layout));

    int num_objects = array_len(layout->layout_objs);

    int num_cols = 0;
    int num_rows = 0;

    // Get the number of row and columns in the grid
    for (int i = 0; i < num_objects; i++) {
        UIObject* obj = get_UIObject(layout->layout_objs, i);
        int rightmost_col = obj->grid_params.col + obj->grid_params.colspan;
        int bottommost_row = obj->grid_params.row + obj->grid_params.rowspan;

        if (rightmost_col > num_cols)
            num_cols = rightmost_col;

        if (bottommost_row > num_rows)
            num_rows = bottommost_row;
    }

    // Compute the row and column sizes
    float col_width = (float)layout->width / (float)num_cols;
    float row_height = (float)layout->height / (float)num_rows;

    // For each object, compute its dimensions and position based on its grid
    // layout params and resize it.
    for (int i = 0; i < num_objects; i++) {
        UIObject* obj = get_UIObject(layout->layout_objs, i);

        float obj_width = col_width * obj->grid_params.colspan;
        float obj_height = row_height * obj->grid_params.rowspan;
        float obj_startx = col_width * obj->grid_params.col;
        float obj_starty = row_height * obj->grid_params.row;

        obj->width = new_ui_float(obj_width, CELLS);
        obj->height = new_ui_float(obj_height, CELLS);
        obj->startx = new_ui_float(obj_startx, CELLS);
        obj->starty = new_ui_float(obj_starty, CELLS);

        obj->resize(layout, obj);
    }

}

void set_layout(UILayout* layout) {
    LOG_FUNC("Running set_layout");

    switch (layout->layout_type) {
        case ROW:
        set_row_layout(layout);
        break;

        case STACK:
        set_stack_layout(layout);
        break;

        case GRID:
        set_grid_layout(layout);
        break;

        default:
        debug_log("Unkown layout type %d\n", layout->layout_type);
    }

    int num_objects = array_len(layout->layout_objs);
    for (int i = 0; i < num_objects; i++) {
        UIObject* obj = get_UIObject(layout->layout_objs, i);
        if (obj->componant == PANE && obj->data.pane != NULL) {
            UIPane* pane = obj->data.pane;
            set_layout(pane->layout);
        }
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
