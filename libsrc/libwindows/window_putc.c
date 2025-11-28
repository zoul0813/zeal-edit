#include "windows.h"
uint8_t window_putc(window_t* w, char c) {
    return window_putc_color(w, c, COLOR(w->fg, w->bg));
}
