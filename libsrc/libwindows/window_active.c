#include <stddef.h>
#include <common.h>
#include "windows.h"
void window_active(window_t* w, uint8_t active) {
    if (w->title == NULL) return;
    uint8_t y = w->y;
    uint8_t x = w->x;
    uint8_t color = active ? COLOR(w->fg_highlight, w->bg) : COLOR(w->fg, w->bg);
    uint8_t len = str_len(w->title) + 4;
    if (w->flags & WIN_TITLE_LEFT) {
    } else if (w->flags & WIN_TITLE_RIGHT) {
        x = x + (w->w - len) - 1;
    } else {
        x = x + ((w->w - len) >> 1);
    }
    text_map_vram();
    for (int i = 0; i < len; i++) {
        x++;
        COLOR_WRITE(w, x, y, color);
    }
    text_demap_vram();
}
