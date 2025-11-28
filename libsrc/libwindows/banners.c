#include <stddef.h>
#include "windows.h"
void _text_banner(uint8_t x, uint8_t y, uint8_t c, uint8_t centered, window_t* w, const char* s) {
    uint8_t width = 0;
    switch (zvb_ctrl_video_mode) {
        case ZVB_CTRL_VID_MODE_TEXT_320: width = 40; break;
        case ZVB_CTRL_VID_MODE_TEXT_640: width = 80; break;
    }
    if (w != NULL) {
        x = w->x + x;
        y = w->y + y;
        if (x > 0) {
            width = w->w - x - w->_attrs.offset;
        } else {
            width = w->w - x;
        }
    } else {
        width -= x;
    }
    uint8_t len = width;
    for (uint8_t i = 0; i < width; i++) {
        if (s[i] == 0x00) {
            len = i;
            break;
        }
    }
    if (len > width)
        len = width;
    char pad = 0;
    if (len > 0) {
        if (centered) {
            pad = (width - len);
            if ((pad % 2) == 0) {
                pad = pad >> 1;
            } else {
                pad = (pad >> 1) - 1;
            }
        }
        text_map_vram();
        for (uint8_t i = 0; i < pad; i++) {
            TEXT_WRITE(win_NonePtr, x, y, CH_SPACE);
            COLOR_WRITE(win_NonePtr, x, y, c);
            x++;
        }
        for (uint8_t i = 0; i < len; i++) {
            TEXT_WRITE(win_NonePtr, x, y, s[i]);
            COLOR_WRITE(win_NonePtr, x, y, c);
            x++;
        }
        for (uint8_t i = 0; i < (width - len - pad); i++) {
            TEXT_WRITE(win_NonePtr, x, y, CH_SPACE);
            COLOR_WRITE(win_NonePtr, x, y, c);
            x++;
        }
        text_demap_vram();
    }
}
void text_banner(uint8_t x, uint8_t y, uint8_t c, uint8_t centered, const char* s) {
    _text_banner(x, y, c, centered, 0, s);
}
void text_header(uint8_t x, uint8_t y, uint8_t c, const char* s) {
    _text_banner(x, y, c, 1, 0, s);
}
void text_menu(uint8_t x, uint8_t y, uint8_t c, const char* items) {
    _text_banner(x, y, c, 0, 0, items);
}

void window_banner(window_t* w, uint8_t x, uint8_t y, uint8_t centered, const char* s) {
    _text_banner(x, y, COLOR(w->fg, w->bg), centered, w, s);
    w->_attrs.pos_y++;
    if (w->_attrs.pos_y < w->y)
        w->_attrs.pos_y = w->y;
}
