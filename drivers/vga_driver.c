#include "vga_driver.h"

/*
 * VGA indexed framebuffer + palette driver for Wizard Core.
 * See docs/vga/overview.md and wizardCore/docs/memory_Map.md.
 */

static uint16_t s_palette_used;
static uint16_t s_palette_keys[VGA_PALETTE_COLORS];

static inline uint16_t palette_key(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0x0Fu) << 8) | ((g & 0x0Fu) << 4) | (b & 0x0Fu));
}

void swap_frame(void) {
    MMIO_STORE32(__vga_swap_addr, 0, 1u);
}

void vga_palette_reset(void) {
    s_palette_used = 0u;
}

uint8_t vga_palette_alloc_color(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t key = palette_key(r, g, b);
    uint8_t i;

    for (i = 0u; i < s_palette_used; ++i) {
        if (s_palette_keys[i] == key) {
            return i;
        }
    }

    if (s_palette_used >= (uint16_t)VGA_PALETTE_COLORS) {
        return 0u;
    }

    i = (uint8_t)s_palette_used;
    s_palette_keys[i] = key;
    s_palette_used = (uint16_t)(s_palette_used + 1u);
    vga_palette_set_fast(i, r, g, b);
    return i;
}

void vga_write_pixel_rgb(uint32_t y, uint32_t x, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }
    vga_write_index_fast(y, x, vga_palette_alloc_color(r, g, b));
}

void vga_fill_row_rgb(uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint32_t count) {
    uint8_t idx = vga_palette_alloc_color(r, g, b);
    if (count > VGA_WIDTH) {
        count = VGA_WIDTH;
    }
    vga_fill_row_index_fast(y, idx, count);
}
