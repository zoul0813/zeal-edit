#include <stdio.h>
#include <zos_video.h>
#include <zvb_hardware.h>
#include <zvb_gfx.h>

#include "conio.h"
#include "windows.h"


static unsigned char window_left = 0;
static unsigned char window_top = 0;
static unsigned char window_right = 0;
static unsigned char window_bottom = 0;
static unsigned char window_x = 0;
static unsigned char window_y = 0;

void window(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom) {
  window_left = left;
  window_top = top;
  window_right = right - 1;
  window_bottom = bottom - 1;
  window_x = window_left;
  window_y = window_top;

  for(unsigned char y = window_top; y <= window_bottom; y++) {
    gotoxy(left, y);
    for(unsigned char x = window_left; x <= window_right; x++) {
      cputc(KEY_SPACE);
    }
  }
}

void window_color(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom, unsigned char color) {
  bgcolor(color);
  window(left, top, right, bottom);
}

void window_box(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom) {
  window(left, top, right, bottom);

  // draw a box
  cputcxy(window_left, window_top, CH_ULCORNER);
  cputcxy(window_right, window_top, CH_URCORNER);
  cputcxy(window_left, window_bottom, CH_LLCORNER);
  cputcxy(window_right, window_bottom, CH_LRCORNER);

  unsigned char width = window_right - window_left;
  unsigned char height = window_bottom - window_top;

  window_y = window_top;
  window_x = window_left + 1;
  for(unsigned char x = 0; x <= width - 2; x++) {
    window_putc(CH_HLINE);
  }

  window_y = window_bottom;
  window_x = window_left + 1;
  for(unsigned char x = 0; x <= width - 2; x++) {
    window_putc(CH_HLINE);
  }

  window_y = window_top + 1;
  for(unsigned char y = 0; y <= height - 2; y++) {
    window_x = window_left;
    window_putc(CH_VLINE);
    window_y++;
  }

  window_y = window_top + 1;
  for(unsigned char y = 0; y <= height - 2; y++) {
    window_x = window_right;
    window_putc(CH_VLINE);
  }

  window_left++;
  window_top++;
  window_right--;
  window_bottom++;
  window_x = window_left;
  window_y = window_top;
}


void window_putc(char c) {
  // unsigned char current_x = zvb_peri_text_curs_x;
  // unsigned char current_y = zvb_peri_text_curs_x;

  switch(c) {
    case CH_NEWLINE:
      window_y++;
      window_x = window_left;
      break;
    default:
      cputcxy(window_x, window_y, c);
  }


  window_x++;
  if(window_x > window_right) { window_x = window_left; window_y++; }
  // we can't do anything about vertical overflow, so just let it happen

  // // reset the cursor
  // gotoxy(current_x, current_y);
}

void window_puts(const char* s) {
  // TODO: arbitrary 256 byte max length?
  for(int i = 0; i < 256; i++) {
    if(s[i] == 0x00) break;
    window_putc(s[i]);
  }
}

void _text_banner(unsigned char x, unsigned char y, unsigned char centered, unsigned char windowed, const char* s) {
  unsigned char bg = (zvb_peri_text_color & 0xF0);
  unsigned char fg = (zvb_peri_text_color & 0x0F);

  unsigned char width, height;
  screensize(&width, &height);
  width -= x;

  if(windowed) {
    x = window_left;
    width = window_right - window_left + 1;
  }

  // invert the colors
  zvb_peri_text_color = fg | bg;

  unsigned char len = width;
  for(unsigned char i = 0; i < width; i++) {
    if(s[i] == 0x00) {
      len = i;
      break;
    }
  }

  char pad = 0;
  if(len > 0) {
    gotoxy(x,y);
    if(centered) {
      pad = (width - len);
      if((pad % 2) == 0) {
        pad = pad >> 1;
      } else {
        pad = (pad >> 1) - 1;
      }
    }

    for(unsigned char i = 0; i < pad; i++) {
      cputc(' ');
    }

    for(unsigned char i = 0; i < len; i++) {
      cputc(s[i]);
    }

    for(unsigned char i = 0; i < (width - len - pad); i++) {
      cputc(' ');
    }
  }

  // invert the colors again
  zvb_peri_text_color = bg | fg;

  // printf("width: %d, height: %d, pad: %d, len: %d\n", width, height, pad, len);
}

void text_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s) {
  _text_banner(x, y, centered, 0, s);
}

void text_header(unsigned char x, unsigned char y, const char* s) {
  _text_banner(x, y, 1, 0, s);
}

void text_menu(unsigned char x, unsigned char y, const char* items) {
  _text_banner(x, y, 0, 0, items);
}

void window_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s) {
  _text_banner(x, y, centered, 1, s);
  window_top++;
  if(window_y < window_top) window_y = window_top;
}