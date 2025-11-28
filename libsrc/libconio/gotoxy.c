#include <zvb_hardware.h>
void gotoxy(unsigned char x, unsigned char y) {
  zvb_peri_text_curs_x = x;
  zvb_peri_text_curs_y = y;
}
