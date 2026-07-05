#ifndef LAYOUT_H
#define LAYOUT_H

#include "../../common/types.h"


void render(UILayout* layout, UIPane* pane);

UILayout* new_layout(int height, int width, LayoutType type);

void free_layout(UILayout* layout);

void set_layout(UILayout* layout);

void register_ui_pane(
    UILayout* layout, UIPane* pane, int id, GridParams* params
);

void register_ui_text(UILayout* layout, UIText* text, int id, GridParams* params);

GridParams new_grid_params(int col, int row, int colspan, int rowspan);

UIObject* new_ui_object(int id, GridParams* params);

void free_ui_object(UIObject* object);

int get_width( UILayout* layout, int id);
int get_height(UILayout* layout, int id);
int get_startx(UILayout* layout, int id);
int get_starty(UILayout* layout, int id);

#endif
