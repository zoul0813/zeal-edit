#include "windows.h"
void window_columns(window_t* w, uint8_t* columns, uint8_t count) {
    uint8_t i, row;
    window(w);
    uint8_t min_y = w->y;
    uint8_t max_y = w->y + w->h - 1;
    text_map_vram();
    for (i = 0; i < count; i++) {
        uint8_t offset = w->x + columns[i];
        for (row = min_y + 1; row < max_y; row++) {
            TEXT_WRITE(w, offset, row, CH_VLINE);
        }
        TEXT_WRITE(w, offset, max_y, CH_TLINEI);
    }
    text_demap_vram();
}
