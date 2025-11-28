#include <zvb_hardware.h>
void highvideo(void) {
  zvb_ctrl_video_mode = ZVB_CTRL_VID_MODE_TEXT_640;
}
