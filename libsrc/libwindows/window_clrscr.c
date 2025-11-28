#include "windows.h"
void window_clrscr(window_t* w) {
    uint8_t color = COLOR(w->fg, w->bg);
    uint8_t x     = w->x + w->_attrs.offset;
    uint8_t y     = w->y + w->_attrs.offset;
    uint8_t min_x = x;
    uint8_t min_y = y;
    uint8_t max_x = (w->x + (w->w - 1)) - w->_attrs.offset;
    uint8_t max_y = (w->y + (w->h - 1)) - w->_attrs.offset;
    text_map_vram();
    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {
            TEXT_WRITE(w, x, y, CH_SPACE);
            COLOR_WRITE(w, x, y, color);
        }
    }
    text_demap_vram();
}
