#ifndef VGA_DRIVER_H
#define VGA_DRIVER_H

#include <stdint.h>
#include "mmio_map.h"

#define VGA_WIDTH           160u
#define VGA_HEIGHT          120u
#define VGA_ROW_ADDR_STRIDE 0x100u
#define VGA_PALETTE_COLORS  256u
/* Reserved for ISA fail screen (solid red via palette, not a raw RGB plane write). */
#define VGA_PAL_INDEX_FAIL_RED 254u

/* Legacy alias: embedded image tools still emit 80 column-bytes per row. */
#define VGA_WIDTH_BYTES     (VGA_WIDTH / 2u)

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

static inline uint32_t vga_fb_addr_fast(uint32_t y, uint32_t x) {
    return VGA_FB_BASE + (y * VGA_ROW_ADDR_STRIDE) + x;
}

static inline uint32_t vga_palette_channel_addr_fast(uint32_t channel_base, uint8_t index) {
    return channel_base + index;
}

static inline void vga_palette_set_fast(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    /* One SB per channel byte; only low nibble is color (high nibble is padding). */
    MMIO_STORE8(__vga_palette_base, index, (uint8_t)(r & 0x0Fu));
    MMIO_STORE8(__vga_pal_green, index, (uint8_t)(g & 0x0Fu));
    MMIO_STORE8(__vga_pal_blue, index, (uint8_t)(b & 0x0Fu));
}

static inline void vga_write_index_fast(uint32_t y, uint32_t x, uint8_t palette_index) {
    MMIO_STORE8(__vga_fb_base, (y * VGA_ROW_ADDR_STRIDE) + x, palette_index);
}

static inline void vga_fill_row_index_fast(uint32_t y, uint8_t palette_index, uint32_t count) {
    uint32_t row_off = y * VGA_ROW_ADDR_STRIDE;
    for (uint32_t x = 0; x < count; ++x) {
        MMIO_STORE8(__vga_fb_base, row_off + x, palette_index);
    }
}

static inline void vga_fill_screen_index_fast(uint8_t palette_index) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        vga_fill_row_index_fast(y, palette_index, VGA_WIDTH);
    }
}

/* Write palette entry then fill every pixel with that index. */
static inline void vga_fill_screen_rgb_fast(uint8_t r, uint8_t g, uint8_t b, uint8_t palette_index) {
    vga_palette_set_fast(palette_index, r, g, b);
    vga_fill_screen_index_fast(palette_index);
}

static inline void vga_fill_screen_fail_red_fast(void) {
    vga_fill_screen_rgb_fast(15u, 0u, 0u, VGA_PAL_INDEX_FAIL_RED);
}

/* Palette index r*16+g with blue fixed at 15 in each entry. */
static inline void vga_init_palette_rg_blue15_fast(void) {
    for (uint8_t r = 0; r < 16u; ++r) {
        for (uint8_t g = 0; g < 16u; ++g) {
            vga_palette_set_fast((uint8_t)((r << 4) | g), r, g, 15u);
        }
    }
}

/* Update blue channel for all 256 palette entries (R/G unchanged). */
static inline void vga_palette_set_blue_all_fast(uint8_t blue) {
    uint8_t b = (uint8_t)(blue & 0x0Fu);
    for (uint16_t i = 0; i < 256u; ++i) {
        MMIO_STORE8(__vga_pal_blue, i, b);
    }
}

static inline uint8_t vga_palette_index_from_rg_fast(uint8_t r, uint8_t g) {
    return (uint8_t)(((r & 0x0Fu) << 4) | (g & 0x0Fu));
}

/* Equal-width 16-level ramps: 160/16 = 10 px per R band, 120/16 = 7.5 px per G band. */
static inline uint8_t vga_gradient_r_equal_band_fast(uint32_t x) {
    return (uint8_t)(15u - ((x << 4) / VGA_WIDTH));
}

static inline uint8_t vga_gradient_g_equal_band_fast(uint32_t y) {
    return (uint8_t)(15u - ((y << 4) / VGA_HEIGHT));
}

static inline uint8_t vga_nibble_from_packed_byte_fast(uint8_t packed, uint32_t x) {
    return (uint8_t)((x & 1u) ? ((packed >> 4) & 0x0Fu) : (packed & 0x0Fu));
}

static inline uint8_t vga_pack_two_pixels_fast(uint8_t even_x, uint8_t odd_x) {
    return (uint8_t)(((odd_x & 0x0Fu) << 4) | (even_x & 0x0Fu));
}

void swap_frame(void);
void vga_palette_reset(void);
uint8_t vga_palette_alloc_color(uint8_t r, uint8_t g, uint8_t b);
void vga_write_pixel_rgb(uint32_t y, uint32_t x, uint8_t r, uint8_t g, uint8_t b);
void vga_fill_row_rgb(uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint32_t count);

#endif
