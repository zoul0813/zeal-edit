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

// #include "demo.h"
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

  // clrscr();
  // cputs("This is just an example of a long string of text.");
  // gotoxy(5, 0);
  // cputs("That was a good");
  // clreol();
  // while(!cgetc()) {}
  // exit(0);

  // demo();
  edit(argv[0]);

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}