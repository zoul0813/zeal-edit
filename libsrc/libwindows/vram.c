#include "windows.h"

static uint8_t mmu_page_current; // wtf???
const __sfr __banked __at(0xF0) mmu_page0_ro;
__sfr __at(0xF0) mmu_page0;

void text_map_vram(void) {
    __asm__("di");
    mmu_page_current = mmu_page0_ro;
    mmu_page0        = VID_MEM_PHYS_ADDR_START >> 14;
}

void text_demap_vram(void) {
    mmu_page0 = mmu_page_current;
    __asm__("ei");
}

