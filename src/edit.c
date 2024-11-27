#include <stdio.h>
#include <string.h>
#include <zos_keyboard.h>
#include <zos_sys.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <zvb_hardware.h>
#include "conio.h"
#include "windows.h"

#include "edit.h"
/** Fake Edit.com Interface */

#define CHUNK_SIZE  1024

#define DOCUMENT_X  0
#define DOCUMENT_Y  1

static unsigned char width = 0, height = 0;
static unsigned char cursor_x = 0, cursor_y = 0;
static unsigned char dialog_open = 0, meta_open = 0;

static const char* filename;
static uint16_t filesize = 0;
static unsigned char file_buffer[32U * 1024U];
static uint16_t buffer_position = 0;
static uint16_t buffer_top = 0;

unsigned char is_ascii(char c) {
  return ((c > 0x1F) && (c < 0x7F));
}

uint16_t previous_line_start(void) {
  if(buffer_top < 1) return 0;
  uint16_t i = 0;
  // buffer_top - 1 (skip the \n)
  for(i = buffer_top - 1; i > 0; i--) {
    if(file_buffer[i] == '\n') break;
  }
  return i; // start of the previous line
}

uint16_t next_line_start(void) {
  uint16_t i = 0;
  for(i = buffer_top; i < filesize; i++) {
    if(file_buffer[i] == '\n') break;
  }
  return i + 1; // start of the next line
}

void draw_topmenu(void) {
  /* draw the main menu */
  bgcolor(TEXT_COLOR_LIGHT_GRAY);
  textcolor(TEXT_COLOR_BLACK);
  text_menu(0, 0, "  Esc=Quit Home/End,PgUp/PgDn,Arrows=Move");
}

void draw_bottomenu(void) {
  /* DRAW THE BOTTOM MENU */
  bgcolor(TEXT_COLOR_CYAN);
  textcolor(TEXT_COLOR_BLACK);
  // why height - 1?
  // text_menu(0, height - 1, "  F1=Help  Enter=Execute  Esc=Cancel  Tab=Next Field");
  char text[80];
  sprintf(text, " Filesize: %05d  Position: %05d  Top: %05d    X: %02d Y: %02d", filesize, buffer_position, buffer_top, cursor_x, cursor_y);
  text_menu(0, height - 1, text);
}

void draw_document(void) {
  /* draw the document window */
  textcolor(TEXT_COLOR_WHITE);
  bgcolor(TEXT_COLOR_DARK_BLUE);
  window_box(DOCUMENT_X, DOCUMENT_Y, width, height);

  /* Draw the window heading */
  unsigned char len = strlen(filename);
  unsigned char left = ((width - len) >> 1) - (len >> 1) - 2;
  cputsxy(left, 1, "[ ");
  cputsxy(left + 2, 1, filename);
  cputsxy(left + len + 2, 1, " ]");

  uint16_t i = buffer_position;
  buffer_top = buffer_position;
  for(i = buffer_position; i < filesize; i++) {
    char c = file_buffer[i];
    window_putc(c);
    if(window_wherey() >= (height - DOCUMENT_Y)) break;
  }

  if(i < filesize) buffer_position = i;
}

void draw_dialog(void) {
  /* draw the dialog */
  unsigned char len = 49; // length of "Find What: ..."
  unsigned char left = (width - len) >> 1;
  unsigned char top = 8;
  window_color(left+1, top+1, left + len + 1, top + 8 + 1, TEXT_COLOR_BLACK);
  window_color(left, top, left + len, top + 8, TEXT_COLOR_LIGHT_GRAY);
  bgcolor(TEXT_COLOR_WHITE);
  textcolor(TEXT_COLOR_BLACK);
  window_banner(left, top, 1, "Find");

  bgcolor(TEXT_COLOR_LIGHT_GRAY);
  textcolor(TEXT_COLOR_BLACK);
  window_puts("\n");
  window_puts(" Find What: [this is the end_\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9]\n");
  window_puts("\n");
  window_puts(" [ ] Match Whole Word Only\n");
  window_puts(" [ ] Match Case\n");

  bgcolor(TEXT_COLOR_DARK_GRAY);
  textcolor(TEXT_COLOR_WHITE);
  window_banner(left, top + 7, 1, "[OK]  [Cancel]  [Help]");
}

void redraw(void) {
  draw_topmenu();
  draw_document();
  if(dialog_open) draw_dialog();
  // draw_bottomenu();

  // textcolor(TEXT_COLOR_BLACK);
  // bgcolor(TEXT_COLOR_WHITE);
}

unsigned char edit_loop(void) {
    draw_bottomenu();
    gotoxy(cursor_x + (DOCUMENT_X + 1), cursor_y + (DOCUMENT_Y + 1));

    char c = cgetc();
    switch(c) {
      case KB_ESC:
        return 1;
        // break; // exit on escape!
      case KB_F1:
        dialog_open ^= 1;
        redraw();
      case KB_INSERT:
        meta_open ^= 1;
        break;
      case KB_HOME:
        buffer_position = 0;
        redraw();
        break;
      case KB_END: {
        buffer_top = filesize;
        unsigned char i = 0;
        uint16_t top = 0;
        for(i = 0; i < height - DOCUMENT_Y - 2; i++) {
          buffer_top = previous_line_start();
        }
        buffer_position = buffer_top;
        redraw();
      } break;
      case KB_PG_UP: {
        buffer_position = buffer_top;
        // find the previous HEIGHT lines
        unsigned char i = 0;
        uint16_t top = 0;
        for(i = 0; i < height - DOCUMENT_Y - 2; i++) {
          buffer_top = previous_line_start();
        }
        buffer_position = buffer_top;
        redraw();
      } break;
      case KB_PG_DOWN:
        buffer_position++; // skip the \n
        redraw();
        break;
      case KB_UP_ARROW:
        if(cursor_y >= 1) cursor_y--;
        else if(buffer_top > 0) {
          uint16_t top = previous_line_start();
          buffer_position = top;
          redraw();
        }
        break;
      case KB_DOWN_ARROW:
        if(cursor_y < (height - 1 - (DOCUMENT_Y + 2))) cursor_y++;
        else if(buffer_position < filesize) {
          uint16_t top = next_line_start();
          buffer_position = top;
          redraw();
        }
        break;
      case KB_LEFT_ARROW:
        if(cursor_x >= 1) cursor_x--;
        break;
      case KB_RIGHT_ARROW:
        if(cursor_x < (width - 1 - (DOCUMENT_X + 2))) cursor_x++;
        break;
      // default:
      //   if(is_ascii(c)) {
      //     cputc(c);
      //     cursor_x++;
      //   }
    }

    return 0;
}

void edit(const char* path) {
  filename = path;

  zos_dev_t dev = open(path, O_RDONLY);
  if(dev < 0) {
    printf("Failed to open file %d (%02x)", -dev, -dev);
    exit(-dev);
  }
  printf("Opened '%s' as %d (%02X)\n", filename, dev, dev);

  uint16_t size = CHUNK_SIZE;
  filesize = 0;
  do {
    zos_err_t err = read(dev, &file_buffer[filesize], &size);
    if(err != ERR_SUCCESS) {
      printf("Failed to read file %d (%02X), size %d \n", err, err, size);
      exit(err);
    }
    printf("Read %d\n", size);
    filesize+=size;
  } while(size == CHUNK_SIZE);
  close(dev);

  screensize(&width, &height);
  // cursor_mode(0x00);

  clr_color(TEXT_COLOR_DARK_BLUE);

  cursor(1);
  cursor_set(177);

  redraw();

  cursor_x = 0;
  cursor_y = 0;

  while(1) {
    unsigned char action = edit_loop();
    if(action) {
      // TODO: implement more actions?
      return;
    }
  }
}