#include "windows.h"
uint8_t window_puts(window_t* w, const char* s) {
    return window_puts_color(w, s, COLOR(w->fg, w->bg));
}
