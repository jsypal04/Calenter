#ifndef PANE_H
#define PANE_H

#include <ncurses.h>

#include "../../common/types.h"

UIPane* new_ui_pane(UILayout* parent_layout, char* title, LayoutType layout_type);

void free_ui_pane(UIPane* pane);

void render_ui_pane(UIObject* object);

void resize_ui_pane(UILayout* parent_layout, UIObject* object);

/**
 * Sets the global active_pane pointer to the specified UIPane reference
 * stored in layout. Returns 0 on success and -1 on error.
 *
 * Fails if the id is not found or if the id refers to a UIObject that
 * does not contain a UIPane object.
 */
int set_active_pane(UILayout* layout, int id);

/**
 * Sets the global active_pane pointer to the next UIPane object in the
 * layout_objs array.
 *
 * Returns a negative number on fail.
 */
int set_next_active_pane(UILayout* layout);

#endif
