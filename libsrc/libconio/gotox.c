#include <zvb_hardware.h>
void gotox(unsigned char x) {
  zvb_peri_text_curs_x = x;
}
