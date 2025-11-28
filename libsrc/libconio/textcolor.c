#include <zvb_hardware.h>
unsigned char textcolor(unsigned char color) {
  uint8_t old = zvb_peri_text_color;
  uint8_t bg = (old & 0xF0);
  uint8_t fg = (color & 0x0F);
  zvb_peri_text_color = (bg | fg);
  return (old & 0x0F);
}
