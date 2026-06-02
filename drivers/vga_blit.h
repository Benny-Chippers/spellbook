#ifndef VGA_BLIT_H
#define VGA_BLIT_H

#include <stdint.h>
#include "vga_driver.h"

/*
 * Indexed-frame helpers: software buffer in normal row-major layout (stride = width),
 * hardware FB uses VGA_ROW_ADDR_STRIDE. Palette entries are loaded from RGB888 tables.
 */

static inline uint8_t vga_rgb888_to_nibble_fast(uint8_t channel) {
    return (uint8_t)((channel >> 4) & 0x0Fu);
}

static inline void vga_palette_load_rgb888_fast(const uint8_t *rgb888) {
    for (uint16_t i = 0; i < VGA_PALETTE_COLORS; ++i) {
        const uint8_t *e = &rgb888[(uint32_t)i * 3u];
        vga_palette_set_fast((uint8_t)i, vga_rgb888_to_nibble_fast(e[0]),
                             vga_rgb888_to_nibble_fast(e[1]),
                             vga_rgb888_to_nibble_fast(e[2]));
    }
}

static inline void vga_present_index_buffer_fast(const uint8_t *indices, uint32_t width) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        uint32_t row = y * width;
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            vga_write_index_fast(x, y, indices[row + x]);
        }
    }
}

static inline void vga_index_buffer_blit_fast(
    uint8_t *dst,
    uint32_t dst_width,
    uint32_t dst_height,
    int32_t dx,
    int32_t dy,
    uint32_t sprite_width,
    uint32_t sprite_height,
    const uint8_t *src,
    uint32_t src_stride,
    uint8_t transparent_index
) {
    for (uint32_t sy = 0; sy < sprite_height; ++sy) {
        int32_t ty = dy + (int32_t)sy;
        if (ty < 0 || ty >= (int32_t)dst_height) {
            continue;
        }
        for (uint32_t sx = 0; sx < sprite_width; ++sx) {
            int32_t tx = dx + (int32_t)sx;
            if (tx < 0 || tx >= (int32_t)dst_width) {
                continue;
            }
            uint8_t index = src[sy * src_stride + sx];
            if (index == transparent_index) {
                continue;
            }
            dst[(uint32_t)ty * dst_width + (uint32_t)tx] = index;
        }
    }
}

static inline void vga_index_buffer_copy_fast(
    uint8_t *dst,
    uint32_t dst_width,
    uint32_t dst_height,
    const uint8_t *src,
    uint32_t src_width,
    uint32_t src_height
) {
    uint32_t copy_w = src_width < dst_width ? src_width : dst_width;
    uint32_t copy_h = src_height < dst_height ? src_height : dst_height;
    for (uint32_t y = 0; y < copy_h; ++y) {
        for (uint32_t x = 0; x < copy_w; ++x) {
            dst[y * dst_width + x] = src[y * src_width + x];
        }
    }
}

#endif
