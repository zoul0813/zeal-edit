#include "conio.h"
void cvlinexy(unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x, y);
  cvline(length);
}
