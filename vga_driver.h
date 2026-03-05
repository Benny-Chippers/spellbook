#ifndef VGA_DRIVER_H
#define VGA_DRIVER_H

#include <stdint.h>

#define VGA_RED_BASE    0x10000000u
#define VGA_GREEN_BASE  0x10010000u
#define VGA_BLUE_BASE   0x10020000u
#define VGA_SWAP_ADDR   0x10030000u

#define VGA_HEIGHT      120u
#define VGA_WIDTH_BYTES 80u
#define VGA_ROW_ADDR_STRIDE 0x100u

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

static inline uint32_t vga_color_addr_fast(uint32_t color_base, uint32_t y, uint32_t x_byte) {
    return color_base + (y * VGA_ROW_ADDR_STRIDE) + x_byte;
}

static inline uint8_t vga_pack_two_pixels_fast(uint8_t even_x, uint8_t odd_x) {
    return (uint8_t)(((odd_x & 0x0Fu) << 4) | (even_x & 0x0Fu));
}

static inline void vga_write_rgb_byte_fast(
    uint32_t y,
    uint32_t x_byte,
    uint8_t red_byte,
    uint8_t green_byte,
    uint8_t blue_byte
) {
    *(volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y, x_byte) = red_byte;
    *(volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y, x_byte) = green_byte;
    *(volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y, x_byte) = blue_byte;
}

static inline void vga_write_single_pixel_byte_fast(const Color *restrict pixels, uint32_t y, uint32_t x_byte) {
    uint8_t red_byte = vga_pack_two_pixels_fast(pixels[0].r, pixels[1].r);
    uint8_t green_byte = vga_pack_two_pixels_fast(pixels[0].g, pixels[1].g);
    uint8_t blue_byte = vga_pack_two_pixels_fast(pixels[0].b, pixels[1].b);
    vga_write_rgb_byte_fast(y, x_byte, red_byte, green_byte, blue_byte);
}

static inline void vga_write_rgb_row_bytes_fast(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict green_row,
    const uint8_t *restrict blue_row,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y, 0u);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y, 0u);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y, 0u);
    for (uint32_t xb = 0; xb < count; ++xb) {
        r[xb] = red_row[xb];
        g[xb] = green_row[xb];
        b[xb] = blue_row[xb];
    }
}

static inline void vga_fill_rgb_row_constant_fast(
    uint32_t y,
    uint8_t red_byte,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y, 0u);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y, 0u);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y, 0u);
    for (uint32_t xb = 0; xb < count; ++xb) {
        r[xb] = red_byte;
        g[xb] = green_byte;
        b[xb] = blue_byte;
    }
}

static inline void vga_write_rgb_row_r_const_gb_fast(
    uint32_t y,
    const uint8_t *restrict red_row,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y, 0u);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y, 0u);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y, 0u);
    for (uint32_t xb = 0; xb < count; ++xb) {
        r[xb] = red_row[xb];
        g[xb] = green_byte;
        b[xb] = blue_byte;
    }
}

static inline void vga_write_rgb_row_rb_const_g_fast(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict blue_row,
    uint8_t green_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y, 0u);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y, 0u);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y, 0u);
    for (uint32_t xb = 0; xb < count; ++xb) {
        r[xb] = red_row[xb];
        g[xb] = green_byte;
        b[xb] = blue_row[xb];
    }
}

static inline void vga_write_rgb_column_bytes_fast(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict green_col,
    const uint8_t *restrict blue_col,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y_start, x_byte);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y_start, x_byte);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y_start, x_byte);
    for (uint32_t i = 0; i < count; ++i) {
        *r = red_col[i];
        *g = green_col[i];
        *b = blue_col[i];
        r = (volatile uint8_t *)((uintptr_t)r + VGA_ROW_ADDR_STRIDE);
        g = (volatile uint8_t *)((uintptr_t)g + VGA_ROW_ADDR_STRIDE);
        b = (volatile uint8_t *)((uintptr_t)b + VGA_ROW_ADDR_STRIDE);
    }
}

static inline void vga_fill_rgb_column_constant_fast(
    uint32_t y_start,
    uint32_t x_byte,
    uint8_t red_byte,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y_start, x_byte);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y_start, x_byte);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y_start, x_byte);
    for (uint32_t i = 0; i < count; ++i) {
        *r = red_byte;
        *g = green_byte;
        *b = blue_byte;
        r = (volatile uint8_t *)((uintptr_t)r + VGA_ROW_ADDR_STRIDE);
        g = (volatile uint8_t *)((uintptr_t)g + VGA_ROW_ADDR_STRIDE);
        b = (volatile uint8_t *)((uintptr_t)b + VGA_ROW_ADDR_STRIDE);
    }
}

static inline void vga_write_rgb_column_r_const_gb_fast(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y_start, x_byte);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y_start, x_byte);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y_start, x_byte);
    for (uint32_t i = 0; i < count; ++i) {
        *r = red_col[i];
        *g = green_byte;
        *b = blue_byte;
        r = (volatile uint8_t *)((uintptr_t)r + VGA_ROW_ADDR_STRIDE);
        g = (volatile uint8_t *)((uintptr_t)g + VGA_ROW_ADDR_STRIDE);
        b = (volatile uint8_t *)((uintptr_t)b + VGA_ROW_ADDR_STRIDE);
    }
}

static inline void vga_write_rgb_column_rb_const_g_fast(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict blue_col,
    uint8_t green_byte,
    uint32_t count
) {
    volatile uint8_t *r = (volatile uint8_t *)vga_color_addr_fast(VGA_RED_BASE, y_start, x_byte);
    volatile uint8_t *g = (volatile uint8_t *)vga_color_addr_fast(VGA_GREEN_BASE, y_start, x_byte);
    volatile uint8_t *b = (volatile uint8_t *)vga_color_addr_fast(VGA_BLUE_BASE, y_start, x_byte);
    for (uint32_t i = 0; i < count; ++i) {
        *r = red_col[i];
        *g = green_byte;
        *b = blue_col[i];
        r = (volatile uint8_t *)((uintptr_t)r + VGA_ROW_ADDR_STRIDE);
        g = (volatile uint8_t *)((uintptr_t)g + VGA_ROW_ADDR_STRIDE);
        b = (volatile uint8_t *)((uintptr_t)b + VGA_ROW_ADDR_STRIDE);
    }
}

uint32_t color_addr(uint32_t color_base, uint32_t y, uint32_t x_byte);
uint8_t pack_two_pixels(uint8_t even_x, uint8_t odd_x);
void swap_frame(void);

void write_single_pixel_byte(const Color *restrict pixels, uint32_t y, uint32_t x_byte);
void write_quad_4_pixels(const Color *restrict block, uint32_t y, uint32_t x_byte);
void write_quad_8_pixels(const Color *restrict block, uint32_t y, uint32_t x_byte);

void write_rgb_byte(uint32_t y, uint32_t x_byte, uint8_t red_byte, uint8_t green_byte, uint8_t blue_byte);
void write_rgb_row_bytes(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict green_row,
    const uint8_t *restrict blue_row,
    uint32_t count
);
void fill_rgb_row_constant(uint32_t y, uint8_t red_byte, uint8_t green_byte, uint8_t blue_byte, uint32_t count);
void write_rgb_row_r_const_gb(
    uint32_t y,
    const uint8_t *restrict red_row,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
);
void write_rgb_row_rb_const_g(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict blue_row,
    uint8_t green_byte,
    uint32_t count
);
void write_rgb_column_bytes(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict green_col,
    const uint8_t *restrict blue_col,
    uint32_t count
);
void fill_rgb_column_constant(
    uint32_t y_start,
    uint32_t x_byte,
    uint8_t red_byte,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
);
void write_rgb_column_r_const_gb(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
);
void write_rgb_column_rb_const_g(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict blue_col,
    uint8_t green_byte,
    uint32_t count
);

#endif
