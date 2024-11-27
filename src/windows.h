#include "conio.h"

void window(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom);
void window_color(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom, unsigned char color);
void window_box(unsigned char left, unsigned char top, unsigned char right, unsigned char bottom);

void window_putc(char c);
void window_puts(const char* s);
unsigned char window_wherex(void);
unsigned char window_wherey(void);

/* Where does this belong? */
void text_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s);
void text_header(unsigned char x, unsigned char y, const char* s);
void text_menu(unsigned char x, unsigned char y, const char* items);


void window_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s);