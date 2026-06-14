#ifndef LAYOUT_H
#define LAYOUT_H

#include "../../common/types.h"



UILayout* init_layout(int height, int width);

void free_layout(UILayout* layout);

void resize_layout(UILayout* layout, int height, int width);

UILayoutObj init_layout_obj(
    int id, float height, float width, float starty, float startx, UIUnit unit
);

void register_obj(UILayout* layout, UILayoutObj obj);

int get_width( UILayout* layout, int id);
int get_height(UILayout* layout, int id);
int get_startx(UILayout* layout, int id);
int get_starty(UILayout* layout, int id);

#endif
