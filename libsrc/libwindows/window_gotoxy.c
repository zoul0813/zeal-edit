#include "windows.h"
inline void window_gotoxy(window_t* w, uint8_t x, uint8_t y) {
    w->_attrs.pos_x = w->x + x + w->_attrs.offset;
    w->_attrs.pos_y = w->y + y + w->_attrs.offset;
}
