#include <keyboard.h>

char cgetc(void) {
  unsigned char c = 0x00;
  do {
    c = getkey();
  } while(c == 0x00);
  return c;
}
