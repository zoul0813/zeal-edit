#include <stdio.h>
#include <zos_video.h>
#include "conio.h"
#include "windows.h"

void main(void) {
  unsigned char width, height;
  screensize(&width, &height);

  clrscr();
  clr_color(TEXT_COLOR_DARK_BLUE);

  gotoxy(10, 10);
  unsigned char fg = textcolor(TEXT_COLOR_RED);
  unsigned char bg = bgcolor(TEXT_COLOR_YELLOW);
  printf("hello world\n");

  gotoxy(0,2);
  textcolor(TEXT_COLOR_CYAN);
  bgcolor(TEXT_COLOR_BLUE);
  cputs("cputs()");

  textcolor(TEXT_COLOR_GREEN);
  bgcolor(bg);
  cputsxy(0, 3, "cputs(0, 3)");

  textcolor(TEXT_COLOR_RED);
  gotoxy(0, 20);
  cputc('A');

  textcolor(TEXT_COLOR_WHITE);
  cputcxy(1, 20, 'B');

  gotoxy(0,1);
  chline(width);

  chlinexy(width >> 2, height - 1, width >> 1);

  window_color(36, 11, 46, 16, 5);
  window_color(35, 10, 44, 15, 4);
  window_puts("Window with wrap");

  textcolor(TEXT_COLOR_BROWN);
  bgcolor(TEXT_COLOR_DARK_GRAY);
  window_box(20, 20, 30, 30);
  window_puts("Boxed In with wrap");

  bgcolor(TEXT_COLOR_LIGHT_GRAY);
  textcolor(TEXT_COLOR_BLACK);
  text_header(0,0, "Text Demo Heading");

  gotoxy(0, height - 1);
}