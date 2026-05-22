/*
 * VGA RGB band sanity check (hardware vs addressing).
 *
 * Fills the 160x120 framebuffer with three horizontal thirds:
 *   rows 0..39   — red only
 *   rows 40..79  — green only
 *   rows 80..119 — blue only
 *
 * Swaps once then holds.
 */

#include <stdint.h>
#include "vga_driver.h"

#define BAND_ROWS (VGA_HEIGHT / 3u)

int main(void) {
    vga_palette_set_fast(0u, 15u, 0u, 0u);
    vga_palette_set_fast(1u, 0u, 15u, 0u);
    vga_palette_set_fast(2u, 0u, 0u, 15u);

    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        if (y < BAND_ROWS) {
            vga_fill_row_index_fast(y, 0u, VGA_WIDTH);
        } else if (y < 2u * BAND_ROWS) {
            vga_fill_row_index_fast(y, 1u, VGA_WIDTH);
        } else {
            vga_fill_row_index_fast(y, 2u, VGA_WIDTH);
        }
    }

    swap_frame();

    for (;;) {
        __asm__ volatile ("nop");
    }
}
