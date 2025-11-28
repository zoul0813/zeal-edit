#include "conio.h"
#include <zvb_hardware.h>
void cputcxy(unsigned char x, unsigned char y, char c) {
  gotoxy(x, y);
  switch(c) {
    case CH_NEWLINE:
      gotoy(++y);
      break;
    default:
      zvb_peri_text_print_char = c;
  }
}
