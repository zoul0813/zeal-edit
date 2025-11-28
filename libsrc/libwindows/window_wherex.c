#include "windows.h"
inline uint8_t window_wherex(window_t* w) {
    return w->_attrs.pos_x - w->_attrs.offset;
}
