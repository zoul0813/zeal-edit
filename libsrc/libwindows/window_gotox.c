#include "windows.h"
inline void window_gotox(window_t* w, uint8_t x) {
    w->_attrs.pos_x = w->x + x + w->_attrs.offset;
}
