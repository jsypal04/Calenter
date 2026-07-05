#ifndef TEXT_H
#define TEXT_H

#include "../../common/types.h"

typedef enum justification_t {
    LEFT   = 0,
    CENTER = 1,
    RIGHT  = 2,
} Justification;

/**
 * Creates a new UIText object
 *
 * NOTE: This function takes ownership of the `content` string. It can
 * be freed after this function returns.
 */
UIText* new_ui_text(char* content, Justification justification);

void free_ui_text(UIText* text);

void render_ui_text(UIPane* pane, UIObject* object);

void resize_ui_text(UILayout* parent_layout, UIObject* obj);

#endif
