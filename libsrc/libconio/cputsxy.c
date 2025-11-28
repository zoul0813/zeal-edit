#include "conio.h"
void cputsxy(unsigned char x, unsigned char y, const char* s) {
  gotoxy(x, y);
  cputs(s);
}
