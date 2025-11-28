#include "conio.h"
void cclearxy(unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x, y);
  cclear(length);
}
