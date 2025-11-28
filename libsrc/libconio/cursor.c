#include <zvb_hardware.h>
unsigned char cursor(unsigned char onoff) {
  static const unsigned char DEFAULT_CURSOR = 30;
  static unsigned char _previous = 1;
  unsigned char _return = _previous;
  _previous = onoff;
  zvb_peri_text_curs_time = onoff ? DEFAULT_CURSOR : 0;
  return _return;
}
