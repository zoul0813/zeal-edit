#include <common.h>
#include "windows.h"

uint8_t MIRROR_TEXT[SCREEN_COL80_HEIGHT][SCREEN_COL80_WIDTH];
uint8_t MIRROR_COLOR[SCREEN_COL80_HEIGHT][SCREEN_COL80_WIDTH];

void window_save(void) {
    text_map_vram();
    mem_cpy(MIRROR_TEXT, SCR_TEXT, sizeof(MIRROR_TEXT));
    mem_cpy(MIRROR_COLOR, SCR_COLOR, sizeof(MIRROR_COLOR));
    text_demap_vram();
}

void window_restore(void) {
    text_map_vram();
    mem_cpy(SCR_TEXT, MIRROR_TEXT, sizeof(MIRROR_TEXT));
    mem_cpy(SCR_COLOR, MIRROR_COLOR, sizeof(MIRROR_COLOR));
    text_demap_vram();
}
