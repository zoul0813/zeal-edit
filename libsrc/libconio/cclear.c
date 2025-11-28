#include "conio.h"
void cclear(unsigned char length) {
  for(unsigned char i = 0; i < length; i++) {
    cputc(CH_SPACE);
  }
}
