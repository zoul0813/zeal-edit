#include "conio.h"
#include <zvb_hardware.h>
void clr_color(unsigned char c) {
  bgcolor(c);
  unsigned char width, height;
  screensize(&width, &height);
  gotoxy(0,0);
  for(unsigned char y = 0; y < height; y++) {
    zvb_peri_text_curs_x = 0;
    zvb_peri_text_curs_y = y;
    for(unsigned char x = 0; x < width; x++) {
      zvb_peri_text_print_char = CH_SPACE;
    }
  }
}
