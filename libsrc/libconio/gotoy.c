#include <zvb_hardware.h>
void gotoy(unsigned char y) {
  zvb_peri_text_curs_y = y;
}
