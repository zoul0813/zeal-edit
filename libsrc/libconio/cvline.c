#include "conio.h"
void cvline(unsigned char length) {
  for(unsigned char i = 0; i < length; i++) {
    cputc(CH_VLINE);
  }
}
