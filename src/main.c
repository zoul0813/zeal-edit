#include <stdio.h>
#include <stdint.h>
#include <zos_sys.h>
#include <zos_keyboard.h>
#include <zos_errors.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <zvb_hardware.h>

#include "conio.h"
#include "windows.h"

#include "edit.h"

/**/
int main(int argc, char** argv) {
  // (void *)argc;
  // (void *)argv;
  zos_err_t err;

  if(argc < 1) {
    printf("Invalid args, provide a filename to edit\n");
    exit(1);
  }

  err = ERR_SUCCESS;

  // demo();
  edit(argv[0]);

  // reset the screen
  ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}