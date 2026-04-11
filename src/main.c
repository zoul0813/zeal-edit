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
  const char* path = NULL;

  if(argc > 0 && argv != NULL && argv[0] != NULL && argv[0][0] != '\0')
    path = argv[0];

  err = ERR_SUCCESS;

  edit(path);

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}
