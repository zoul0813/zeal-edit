#include <stdio.h>
#include <string.h>
#include <zos_video.h>
#include <zvb_hardware.h>
#include <zvb_gfx.h>

#include "conio.h"
#include "windows.h"

void window(window_t* window) {
  window->_attrs.offset = (window->flags & WIN_BORDER) ? 1 : 0;
  window->_attrs.pos_x = window->x + window->_attrs.offset;
  window->_attrs.pos_y = window->y + window->_attrs.offset;

  textcolor(window->fg);
  bgcolor(window->bg);

  unsigned char cur = cursor(0);

  unsigned char y = window->y;
  unsigned char x = window->x;
  unsigned char max_x = window->x + window->w;
  unsigned char max_y = window->y + window->h;
  unsigned char bordered = (window->flags & WIN_BORDER) > 0 ? 1 : 0;
  for(y = window->y; y < max_y; y++) {
    zvb_peri_text_curs_y = y;
    for(x = window->x; x < max_x; x++) {
      char c = KEY_SPACE;
      // OPTIMIZE: move this into a separate block with loops like before...?
      if(bordered) {
        if(x == window->x && y == window->y) c = CH_ULCORNER;
        else if(x == max_x - 1 && y == window->y) c = CH_URCORNER;
        else if(x == window->x && y == max_y - 1) c = CH_LLCORNER;
        else if(x == max_x - 1 && y == max_y - 1) c = CH_LRCORNER;
        else if(x == window->x) c = CH_VLINE;
        else if(x == max_x - 1) c = CH_VLINE;
        else if(y == window->y) c = CH_HLINE;
        else if(y == max_y - 1) c = CH_HLINE;
      }
      zvb_peri_text_curs_x = x;
      // cputc(KEY_SPACE);
      zvb_peri_text_print_char = c;
    }
  }

  if(window->title != NULL) {
    /* Draw the window heading */
    unsigned char len = strlen(window->title);
    unsigned char left = ((window->w - len) >> 1) - (len >> 1) - 2;
    zvb_peri_text_curs_x = left;
    zvb_peri_text_curs_y = window->y;
    // cputsxy(left, 1, "[ ");
    zvb_peri_text_print_char = '[';
    zvb_peri_text_print_char = ' ';
    cputs(window->title);
    zvb_peri_text_print_char = ' ';
    zvb_peri_text_print_char = ']';
  }

  if((window->flags & WIN_SHADOW) > 0) {
    // draw the shadow
    // let's assume all shadows are black for now?
    bgcolor(TEXT_COLOR_BLACK);
    for(unsigned char i = 0; i < window->h; i++) {
      zvb_peri_text_curs_x = window->x + window->w;
      zvb_peri_text_curs_y = window->y + 1 + i;
      zvb_peri_text_print_char = ' ';
    }

    zvb_peri_text_curs_x = window->x + 1;
    for(unsigned char i = 0; i < window->w; i++) {
      zvb_peri_text_print_char = ' ';
    }
  }

  cursor(cur);
}

void window_gotox(window_t* window, unsigned char x) {
  window->_attrs.pos_x = window->x + x + window->_attrs.offset;
}

void window_gotoy(window_t* window, unsigned char y) {
  window->_attrs.pos_y = window->y + y + window->_attrs.offset;
}
void window_gotoxy(window_t* window, unsigned char x, unsigned char y) {
  window->_attrs.pos_x = window->x + x + window->_attrs.offset;
  window->_attrs.pos_y = window->y + y + window->_attrs.offset;
}

void window_clrscr(window_t *window) {
  unsigned char current_x = window->_attrs.pos_x;
  unsigned char current_y = window->_attrs.pos_y;
  unsigned char current_color = zvb_peri_text_color;

  textcolor(window->fg);
  bgcolor(window->bg);

  unsigned char max_w = window->w - (window->_attrs.offset * 2);
  unsigned char max_h = window->h - (window->_attrs.offset * 2);
  unsigned char lines = 0;

  window_gotoxy(window, 0, 0);
  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  for(unsigned char y = 0; y < max_h; y++) {
    for(unsigned char x = 0; x < max_w; x++) {
      zvb_peri_text_print_char = ' ';
    }
    zvb_peri_text_curs_x = window->x + window->_attrs.offset;
    zvb_peri_text_curs_y += 1;
  }

  // reset color
  zvb_peri_text_color = current_color;
  window->_attrs.pos_x = current_x;
  window->_attrs.pos_y = current_y;
  zvb_peri_text_curs_x = current_x;
  zvb_peri_text_curs_y = current_y;
}

void window_clreol(window_t *window) {
  unsigned char current_color = zvb_peri_text_color;

  textcolor(window->fg);
  bgcolor(window->bg);

  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  unsigned char max_w = window->w - window->_attrs.offset;
  for(unsigned char x = window->_attrs.pos_x; x < max_w; x++) {
    zvb_peri_text_print_char = ' ';
  }

  // reset color
  zvb_peri_text_color = current_color;
  window->_attrs.pos_x = window->x + window->_attrs.offset;
  window->_attrs.pos_y++;
  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;
}

unsigned char window_wherex(window_t* window) {
  return window->_attrs.pos_x - window->_attrs.offset;
}

unsigned char window_wherey(window_t* window) {
  return window->_attrs.pos_y - window->_attrs.offset;
}

unsigned char window_putc(window_t* window, char c) {
  //save color
  unsigned char current_color = zvb_peri_text_color;

  textcolor(window->fg);
  bgcolor(window->bg);

  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  unsigned char lines = 0;

  switch(c) {
    case CH_NEWLINE:
      window->_attrs.pos_y++;
      window->_attrs.pos_x = window->x + window->_attrs.offset;
      lines++;
      break;
    case CH_TAB:
      unsigned char tab_width = (window->_attrs.pos_x - window->_attrs.offset) % 4;
      if(tab_width == 0) tab_width = 4;
      for(unsigned char i = 0; i < tab_width; i++) {
        zvb_peri_text_print_char = ' ';
      }
      window->_attrs.pos_x += tab_width;
      break;
    default:
      zvb_peri_text_print_char = c;
      window->_attrs.pos_x++;
  }

  if(window->_attrs.pos_x > ((window->x + window->w - 1) - window->_attrs.offset)) {
    window->_attrs.pos_x = window->x + window->_attrs.offset;
    window->_attrs.pos_y++;
    lines++;
  }
  // we can't do anything about vertical overflow, so just let it happen

  // reset color
  zvb_peri_text_color = current_color;

  return lines;
}

unsigned char window_puts(window_t* window, const char* s) {
  // TODO: arbitrary 256 byte max length?
  unsigned char lines = 0;
  for(int i = 0; i < 256; i++) {
    if(s[i] == 0x00) break;
    lines += window_putc(window, s[i]);
  }
  return lines;
}

void _text_banner(unsigned char x, unsigned char y, unsigned char centered, window_t* window, const char* s) {
  unsigned char bg = (zvb_peri_text_color & 0xF0);
  unsigned char fg = (zvb_peri_text_color & 0x0F);
  unsigned char cur = cursor(0);

  unsigned char width, height;
  screensize(&width, &height);

  if(window != NULL) {
    x = window->x + x;
    y = window->y + y;
    width = (window->x + window->w - 1) - window->x + 1;
  } else {
    width -= x;
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
    // gotoxy(x,y);
    zvb_peri_text_curs_x = x;
    zvb_peri_text_curs_y = y;
    if(centered) {
      pad = (width - len);
      if((pad % 2) == 0) {
        pad = pad >> 1;
      } else {
        pad = (pad >> 1) - 1;
      }
    }

    for(unsigned char i = 0; i < pad; i++) {
      // cputc(' ');
      zvb_peri_text_print_char = ' ';
    }

    for(unsigned char i = 0; i < len; i++) {
      // cputc(s[i]);
      zvb_peri_text_print_char = s[i];
    }

    for(unsigned char i = 0; i < (width - len - pad); i++) {
      // cputc(' ');
      zvb_peri_text_print_char = ' ';
    }
  }

  // invert the colors again
  zvb_peri_text_color = bg | fg;

  cursor(cur);
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

void window_banner(window_t* window, unsigned char x, unsigned char y, unsigned char centered, const char* s) {
  _text_banner(x, y, centered, window, s);
  window->_attrs.pos_y++;
  if(window->_attrs.pos_y < window->y) window->_attrs.pos_y = window->y;
}