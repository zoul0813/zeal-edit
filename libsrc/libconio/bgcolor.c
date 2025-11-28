#include <zvb_hardware.h>
unsigned char bgcolor(unsigned char color) {
  uint8_t old = zvb_peri_text_color;
  uint8_t bg = ((color << 4) & 0xF0);
  uint8_t fg = (old & 0x0F);
  zvb_peri_text_color = (bg | fg);
  return (old & 0x0F) >> 4;
}
