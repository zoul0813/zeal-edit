#include "windows.h"
inline void window_gotoy(window_t* w, uint8_t y) {
    w->_attrs.pos_y = w->y + y + w->_attrs.offset;
}
