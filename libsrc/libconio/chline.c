#include "conio.h"
void chline(unsigned char length) {
  for(unsigned char i = 0; i < length; i++) {
    cputc(CH_HLINE);
  }
}
