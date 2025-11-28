#include "windows.h"
inline uint8_t window_wherey(window_t* w) {
    return w->_attrs.pos_y - w->_attrs.offset;
}
