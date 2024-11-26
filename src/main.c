#include <stdio.h>
#include <stdint.h>
#include <zos_sys.h>
#include <zos_keyboard.h>
#include <zos_errors.h>
#include <zos_video.h>
#include <zos_vfs.h>

#include "conio.h"
#include "windows.h"

#include "demo.h"
#include "edit.h"

/**/
int main(int argc, char** argv) {
  (void *)argc;
  (void *)argv;
  zos_err_t err;

  // /* Set keyboard mode for conio ??? */
  // void* arg = (void*) (KB_READ_BLOCK | KB_MODE_HALFCOOKED);
  // err = ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);
  // if(err != ERR_SUCCESS) {
  //   printf("Failed to set keyboard mode %d (%02x)\n", err, err);
  //   exit(err);
  // }

  err = ERR_SUCCESS;

  // demo();
  edit();

  // char key[32] = { 0x00 };
  // do {
  //   uint16_t size = sizeof(key);
  //   err = read(DEV_STDIN, key, &size);
  //   if(err != ERR_SUCCESS) {
  //     printf("keyboard error: %d (%02x)\n", err, err);
  //   }

  //   if(size > 0) break;
  // } while(1);

  // reset the screen
  ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}