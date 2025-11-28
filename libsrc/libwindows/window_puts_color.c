#include "windows.h"
uint8_t window_puts_color(window_t* w, const char* s, uint8_t color) {
    uint8_t lines = 0;
    for (int i = 0; i < 256; i++) {
        if (s[i] == 0x00)
            break;
        lines += window_putc_color(w, s[i], color);
    }
    return lines;
}
