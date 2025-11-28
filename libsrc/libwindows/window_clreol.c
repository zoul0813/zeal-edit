#include "windows.h"
void window_clreol(window_t* w) {
    uint8_t color = COLOR(w->fg, w->bg);
    uint8_t x     = w->_attrs.pos_x;
    uint8_t y     = w->_attrs.pos_y;
    uint8_t min_x = x;
    uint8_t max_x = (w->x + (w->w - 1)) - w->_attrs.offset;
    text_map_vram();
    for (x = min_x; x <= max_x; x++) {
        TEXT_WRITE(w, x, y, CH_SPACE);
        COLOR_WRITE(w, x, y, color);
    }
    text_demap_vram();
    w->_attrs.pos_x = w->x + w->_attrs.offset;
    w->_attrs.pos_y++;
}
