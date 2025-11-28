#include "windows.h"
uint8_t window_putc_color(window_t* w, char c, uint8_t color) {
    uint8_t x     = w->_attrs.pos_x;
    uint8_t min_x = x;
    uint8_t y     = w->_attrs.pos_y;
    uint8_t lines = 0;
    uint8_t tab_width = ((w->_attrs.pos_x - w->_attrs.offset) - w->x) % 4;
    text_map_vram();
    switch (c) {
        case CH_NEWLINE:
            w->_attrs.pos_y = ++y;
            w->_attrs.pos_x = x = w->x + w->_attrs.offset;
            lines++;
            break;
        case CH_TAB:
            if (tab_width == 0)
                tab_width = 4;
            for (x = min_x; x < tab_width; x++) {
                TEXT_WRITE(w, x, y, CH_SPACE);
                COLOR_WRITE(w, x, y, color);
            }
            w->_attrs.pos_x += tab_width;
            break;
        default:
            TEXT_WRITE(w, x, y, c);
            COLOR_WRITE(w, x, y, color);
            w->_attrs.pos_x++;
    }
    text_demap_vram();
    if (w->_attrs.pos_x > ((w->x + w->w - 1) - w->_attrs.offset)) {
        w->_attrs.pos_x = w->x + w->_attrs.offset;
        w->_attrs.pos_y++;
        lines++;
    }
    return lines;
}
