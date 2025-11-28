#include "conio.h"
#include <zvb_hardware.h>
void clreol(void) {
  unsigned char x = zvb_peri_text_curs_x;
  unsigned char y = zvb_peri_text_curs_y;
  unsigned char width, height;
  unsigned char i = x;
  screensize(&width, &height);
  for(i = x; i < width; i++) {
    zvb_peri_text_print_char = ' ';
  }
  zvb_peri_text_curs_x = 0;
  zvb_peri_text_curs_y = y + 1;
}
