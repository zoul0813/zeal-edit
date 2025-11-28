#include <zvb_hardware.h>
void cputc(char c) {
  zvb_peri_text_print_char = c;
}
