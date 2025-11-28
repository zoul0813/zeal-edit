#include "conio.h"
void chlinexy(unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x, y);
  chline(length);
}
