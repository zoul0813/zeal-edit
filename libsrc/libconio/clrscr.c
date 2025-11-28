#include <stddef.h>
#include <zos_vfs.h>
#include <zos_video.h>
void clrscr(void) {
  ioctl(DEV_STDOUT, CMD_CLEAR_SCREEN, NULL);
}
