#include <stdint.h>
#include <zos_sys.h>
#include <zos_errors.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <core.h>
#include <windows.h>
#include <conio.h>

#include "edit.h"

/**/
int main(int argc, char** argv) {
  zos_err_t err;

  if(argc < 1) {
    put_s("Invalid args, provide a filename to edit\n");
    exit(1);
  }

  err = ERR_SUCCESS;

  edit(argv[0]);

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}