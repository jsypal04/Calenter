#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "../calenter.h"
#include "../layout/layout.h"
#include "../utils/debug.h"
#include "../../common/types.h"


typedef struct ui_text_t {
    char* value;
    Justification justification;
} UIText;

UIText* new_ui_text(char* content, Justification justification) {
    UIText* text = malloc(sizeof(UIText));
    text->justification = justification;
    text->value = strdup(content);
    return text;
}

void free_ui_text(UIText* text) {
    LOG_FUNC("Running free_ui_text");
    free(text->value);
    text->value = NULL;
    free(text);
    text = NULL;
}

void resize_ui_text(UILayout* parent_layout, UIObject* obj) {
    return;

    // assert(obj->componant == TEXT);
    // assert(obj->data.text != NULL);

    // UIText* text = obj->data.text;

}

void render_ui_text(UIPane* pane, UIObject* object) {
    LOG_FUNC("Running render_ui_text");
    assert(object->componant == TEXT);
    assert(object->data.text != NULL);

    UIText* text = object->data.text;

    if (text->value == NULL) return;

    int text_length = strlen(text->value);
    int width = get_width(pane->layout, object->id);

    int starty = get_starty(pane->layout, object->id);
    starty++;

    int line_number = 0;
    int buf_index = 0;
    char* line_beginning = text->value;

    if (text_length >= width) {
        int startx = get_startx(pane->layout, object->id);
        startx++;
        char* tmp_buf = malloc(sizeof(char) * width);
        do {
            bzero(tmp_buf, sizeof(char) * width);

            int buf_size =
                text_length - buf_index < width - 2 ?
                text_length - buf_index :
                width - 2;

            debug_log("startx = %d\n", startx);
            strncpy(tmp_buf, line_beginning, buf_size);
            trim(tmp_buf);

            if (buf_size < width - 2) {
                switch (text->justification) {
                    case CENTER:
                    startx = (width - buf_size) / 2;
                    break;

                    case RIGHT:
                    startx = width - buf_size;
                    break;

                    case LEFT:
                    // This is the default behavior. Including case to make LSP happy
                    break;
                }
            }

            mvwprintw(pane->win, starty + line_number, startx, "%s", tmp_buf);

            line_beginning += buf_size;
            buf_index += buf_size;
            line_number++;
        } while (buf_index < text_length);
        free(tmp_buf);
        tmp_buf = NULL;
        return;
    }

    int startx = -1;

    switch (text->justification) {
        case LEFT:
        startx = get_startx(pane->layout, object->id);
        break;

        case RIGHT:
        startx = width - text_length - 2;
        break;

        case CENTER:
        startx = (width - text_length) / 2;
        break;
    }

    assert(startx != -1);

    startx++;

    mvwprintw(
        pane->win,
        starty,
        startx,
        "%s", text->value
    );
}
