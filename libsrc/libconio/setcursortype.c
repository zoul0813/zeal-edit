#include <zvb_hardware.h>
void setcursortype(unsigned char c) {
  zvb_peri_text_curs_char = c;
}
