#ifndef PANE_H
#define PANE_H

#include <ncurses.h>

#include "../../common/types.h"

UIPane* new_ui_pane(UILayout* parent_layout, int id, char* title);

void free_ui_pane(UIPane* pane);

void render_ui_pane(UIObject* object);

#endif
