#include <zvb_hardware.h>
void screensize(unsigned char* x, unsigned char* y) {
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
