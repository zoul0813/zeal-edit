#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include "common.h"

uint16_t put_u16(uint16_t i) {
    char c[3];
    uint16_t size;
    zos_err_t err;
    uint8_t h = (uint8_t)(i >> 8 & 0xFF);
    uint8_t l = (uint8_t)(i & 0xFF);

    u8tohex(h, c, 'A');
    size = 3;
    err = write(DEV_STDOUT, &c, &size);
    if(err != ERR_SUCCESS) exit(err);

    u8tohex(l, c, 'A');
    size = 3;
    err = write(DEV_STDOUT, &c, &size);
    if(err != ERR_SUCCESS) exit(err);

    return 4;
}