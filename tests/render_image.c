#include <stdint.h>

#include "vga_driver.h"

/*
 * Static bitmap renderer — pairs with generated *_image.c / *_image.h data.
 *
 * Embedded data is row-major packed RGB444 source data: per column-byte,
 * three channel bytes each holding two 4-bit pixels (even X low nibble, odd X high).
 * The renderer converts each source color to a palette index before writing the framebuffer.
 */
#include "vga_image_link.h"

int main(void) {
    vga_palette_reset();

    for (uint32_t y = 0u; y < VGA_HEIGHT; y++) {
        uint32_t row_base = y * VGA_WIDTH_BYTES * 3u;
        for (uint32_t xb = 0u; xb < VGA_WIDTH_BYTES; xb++) {
            uint32_t idx = row_base + xb * 3u;
            uint8_t r_byte = VGA_IMAGE_ARRAY[idx];
            uint8_t g_byte = VGA_IMAGE_ARRAY[idx + 1u];
            uint8_t b_byte = VGA_IMAGE_ARRAY[idx + 2u];
            uint32_t x0 = xb << 1;

            vga_write_pixel_rgb(
                x0,
                y,
                vga_nibble_from_packed_byte_fast(r_byte, x0),
                vga_nibble_from_packed_byte_fast(g_byte, x0),
                vga_nibble_from_packed_byte_fast(b_byte, x0)
            );
            vga_write_pixel_rgb(
                x0 + 1u,
                y,
                vga_nibble_from_packed_byte_fast(r_byte, x0 + 1u),
                vga_nibble_from_packed_byte_fast(g_byte, x0 + 1u),
                vga_nibble_from_packed_byte_fast(b_byte, x0 + 1u)
            );
        }
    }
    swap_frame();

    for (;;) {
        /* idle */
    }
}
