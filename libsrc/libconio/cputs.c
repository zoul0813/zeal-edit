#include <zvb_hardware.h>
void cputs(const char* s) {
  for(int i = 0; i < 256; i++) {
    if(s[i] == 0x00) break;
    zvb_peri_text_print_char = s[i];
  }
}
