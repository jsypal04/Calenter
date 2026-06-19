#ifndef LAYOUT_H
#define LAYOUT_H

#include "../../common/types.h"


void render(UILayout* layout);

UILayout* new_layout(int height, int width, LayoutType type);

void free_layout(UILayout* layout);

void resize_layout(UILayout* layout, int height, int width);

UIObject* new_ui_object(int id);

void free_ui_object(UIObject* object);

void register_obj(UILayout* layout, UIObject* obj);

int get_width( UILayout* layout, int id);
int get_height(UILayout* layout, int id);
int get_startx(UILayout* layout, int id);
int get_starty(UILayout* layout, int id);

#endif
