#include <zvb_hardware.h>
void setcursormode(unsigned char m) {
  zvb_peri_text_ctrl = m;
}
