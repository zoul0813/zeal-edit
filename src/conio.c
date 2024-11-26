#include <stdio.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include <zvb_hardware.h>
#include <zvb_gfx.h>

#include "conio.h"

// DIRTY: trick to align key_buffer
static unsigned char __array[32 + 31] = { 0x00 };
static unsigned char* __key_buffer;

zos_err_t __kb_flush(void) {
  /* Flush the keyboard fifo */
  uint16_t size = sizeof(__key_buffer);
  while (size) {
    zos_err_t err = read(DEV_STDIN, __key_buffer, &size);
    if(err != ERR_SUCCESS) return err;
  }
  return ERR_SUCCESS;
}

/**
 * Initializes the keyboard
 */
zos_err_t __kb_mode(unsigned char mode) {
  /* Initialize the keyboard by setting it to raw and non-blocking */
  void* arg = (void *)mode; // (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
  zos_err_t err = ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);

  __key_buffer = (uint8_t*)((uintptr_t)(__array + 31) & (~(uintptr_t)31));

  return err;
}


/* Clear the whole screen and put the cursor into the top left corner */
void clrscr(void) {
  zos_err_t err = ioctl(DEV_STDOUT, CMD_CLEAR_SCREEN, NULL);
}

/* Clear the screen with a specified bgcolor */
void clr_color(unsigned char c) {
  bgcolor(c);
  unsigned char width, height;
  screensize(&width, &height);
  gotoxy(0,0);
  for(unsigned char y = 0; y < height; y++) {
    for(unsigned char x = 0; x < width; x++) {
      zvb_peri_text_print_char = KEY_SPACE;
    }
  }
}

/* Return true if there's a key waiting, return false if not */
unsigned char kbhit (void) {
  return 0;
}

/* Set the cursor to the specified X position, leave the Y position untouched */
void gotox (unsigned char x) {
  zvb_peri_text_curs_x = x;
}

/* Set the cursor to the specified Y position, leave the X position untouched */
void gotoy (unsigned char y) {
  zvb_peri_text_curs_y = y;
}

/* Set the cursor to the specified position */
void gotoxy (unsigned char x, unsigned char y) {
  zvb_peri_text_curs_x = x;
  zvb_peri_text_curs_y = y;
}

/* Return the X position of the cursor */
unsigned char wherex (void) {
  return zvb_peri_text_curs_x;
}

/* Return the Y position of the cursor */
unsigned char wherey (void) {
  return zvb_peri_text_curs_y;
}

/* Output one character at the current cursor position */
void cputc (char c) {
  // uint16_t size = 1;
  // write(DEV_STDOUT, &c, &size);
  // putc(c, DEV_STDOUT);
  zvb_peri_text_print_char = c;
}

/* Same as "gotoxy (x, y); cputc (c); */
void cputcxy (unsigned char x, unsigned char y, char c) {
  gotoxy(x, y);
  // cputc(c);
  switch(c) {
    case CH_NEWLINE:
      gotoy(++y);
      break;
    default:
      zvb_peri_text_print_char = c;
  }
}

/* Output a NUL-terminated string at the current cursor position */
void cputs (const char* s) {
  puts(s);
}

/* Same as "gotoxy (x, y); puts (s); */
void cputsxy (unsigned char x, unsigned char y, const char* s) {
  gotoxy(x,y);
  cputs(s);
}

// /* Like printf(), but uses direct screen output */
// int cprintf (const char* format, ...) {
//   return 0;
// }

// /* Like vprintf(), but uses direct screen output */
// int vcprintf (const char* format, va_list ap) {
//   return 0;
// }

/* Return a character from the keyboard. If there is no character available,
** the function waits until the user does press a key. If cursor is set to
** 1 (see below), a blinking cursor is displayed while waiting.
*/
char cgetc (void) {
  __kb_mode(KB_READ_NON_BLOCK | KB_MODE_RAW);
  __kb_flush();

  // unsigned char i = 0;
  unsigned char c = 0x00;
  do {
    uint16_t size = sizeof(__key_buffer);
    zos_err_t err = read(DEV_STDIN, __key_buffer, &size);
    if(err != ERR_SUCCESS) {
      printf("keyboard error: %d (%02x)\n", err, err);
      exit(err);
    }

    gotoxy(0, 30);
    if(size > 0) {
      for(uint16_t i = 0; i < size; i++) {
        c = __key_buffer[i];
        if(c == KB_RELEASED) {
          i++;
          continue;
        }
        goto _exit_cgetc;
      }
    }
  } while(1);
_exit_cgetc:
  return c;
}

// /* Like scanf(), but uses direct keyboard input */
// int cscanf (const char* format, ...) {
//   return 0;
// }

// /* Like vscanf(), but uses direct keyboard input */
// int vcscanf (const char* format, va_list ap) {
//   return 0;
// }

// /* Return the character from the current cursor position */
// char cpeekc (void) {
//   return 0;
// }

// /* Return the color from the current cursor position */
// unsigned char cpeekcolor (void) {
//   return 0;
// }

// /* Return the reverse attribute from the current cursor position.
// ** If the character is reversed, then return 1; return 0 otherwise.
// */
// unsigned char cpeekrevers (void) {
//   return 0;
// }

// /* Return a string of the characters that start at the current cursor position.
// ** Put the string into the buffer to which "s" points.  The string will have
// ** "length" characters, then will be '\0'-terminated.
// */
// void cpeeks (char* s, unsigned int length) {
  
// }

/* If onoff is 1, a cursor is displayed when waiting for keyboard input. If
** onoff is 0, the cursor is hidden when waiting for keyboard input. The
** function returns the old cursor setting.
*/
unsigned char cursor (unsigned char onoff) {
  static const unsigned char DEFAULT_CURSOR = 30;
  static unsigned char _previous = 1;
  unsigned char _return = _previous;
  _previous = onoff;

  zvb_peri_text_curs_time = onoff ? DEFAULT_CURSOR : 0;

  return _return;
}

// /* Enable/disable reverse character display. This may not be supported by
// ** the output device. Return the old setting.
// */
// unsigned char revers (unsigned char onoff) {
//   return 0;
// }

/* Set the color for text output. The old color setting is returned. */
unsigned char textcolor (unsigned char color) {
  uint8_t old = zvb_peri_text_color;

  uint8_t bg = (old & 0xF0);
  uint8_t fg = (color & 0x0F);

  zvb_peri_text_color = (bg | fg);

  return (old & 0x0F);
}

/* Set the color for the background. The old color setting is returned. */
unsigned char bgcolor (unsigned char color) {
  uint8_t old = zvb_peri_text_color;

  uint8_t bg = ((color << 4) & 0xF0);
  uint8_t fg = (old & 0x0F);

  zvb_peri_text_color = (bg | fg);

  return (old & 0x0F) >> 4;
}

// /* Set the color for the border. The old color setting is returned. */
// unsigned char bordercolor (unsigned char color) {
//   return 0;
// }

/* Output a horizontal line with the given length starting at the current
** cursor position.
*/
void chline (unsigned char length) {
  for(unsigned char i = 0; i < length; i++) {
    cputc(CH_HLINE);
  }
}

/* Same as "gotoxy (x, y); chline (length);" */
void chlinexy (unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x,y);
  chline(length);
}

/* Output a vertical line with the given length at the current cursor
** position.
*/
void cvline (unsigned char length) {
  for(unsigned char i = 0; i < length; i++) {
    cputc(CH_VLINE);
  }
}

/* Same as "gotoxy (x, y); cvline (length);" */
void cvlinexy (unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x,y);
  cvline(length);
}

/* Clear part of a line (write length spaces). */
void cclear (unsigned char length) {
    for(unsigned char i = 0; i < length; i++) {
    cputc(KEY_SPACE);
  }
}

/* Same as "gotoxy (x, y); cclear (length);" */
void cclearxy (unsigned char x, unsigned char y, unsigned char length) {
  gotoxy(x,y);
  cclear(length);
}

/* Return the current screen size. */
void screensize (unsigned char* x, unsigned char* y) {
  // TODO: define these values in zvb_hardware.h with a macro?

  // zos_text_area_t area;
  // zos_err_t err = ioctl(DEV_STDOUT, CMD_GET_AREA, &area);

  switch(zvb_ctrl_video_mode) {
    case ZVB_CTRL_VID_MODE_TEXT_320:
      *x = 40;
      *y = 20;
      break;
    case ZVB_CTRL_VID_MODE_TEXT_640:
      *x = 80;
      *y = 40;
      break;
  }
}