#include "vga_driver.h"

/*
 * VGA interface validation test for Wizard Core CPU.
 *
 * Address map (from Capstone/docs/memory_Map.md):
 *   0x1000_0000 - 0x1000_774F : Red channel bytes
 *   0x1001_0000 - 0x1001_774F : Green channel bytes
 *   0x1002_0000 - 0x1002_774F : Blue channel bytes
 *   0x1003_0000              : Frame buffer swap trigger (write)
 *
 * Important layout details:
 * - 160x120 pixels are packed as 80 bytes per row (2 pixels/byte).
 * - Low nibble is even X pixel, high nibble is odd X pixel.
 * - Addressing is not contiguous per pixel RGB tuple: each color plane is separate.
 */

// FUNCTIONS

uint32_t color_addr(uint32_t color_base, uint32_t y, uint32_t x_byte) {
    return vga_color_addr_fast(color_base, y, x_byte);
}

uint8_t pack_two_pixels(uint8_t even_x, uint8_t odd_x) {
    return vga_pack_two_pixels_fast(even_x, odd_x);
}

void swap_frame(void) {
    *(volatile uint32_t *)VGA_SWAP_ADDR = 1u;
}

static inline uint16_t pack_four_pixels_16(uint8_t px_1, uint8_t px_2, uint8_t px_3, uint8_t px_4) {
    return (uint16_t)(((px_4 & 0x0Fu) << 12) | ((px_3 & 0x0Fu) << 8) | ((px_2 & 0x0Fu) << 4) | (px_1 & 0x0Fu));
}

static inline uint32_t pack_eight_pixels_32(uint8_t px_1, uint8_t px_2, uint8_t px_3, uint8_t px_4, uint8_t px_5, uint8_t px_6, uint8_t px_7, uint8_t px_8) {
    return (uint32_t)(((px_8 & 0x0Fu) << 28) | ((px_7 & 0x0Fu) << 24) | ((px_6 & 0x0Fu) << 20) | ((px_5 & 0x0Fu) << 16) | ((px_4 & 0x0Fu) << 12) | ((px_3 & 0x0Fu) << 8) | ((px_2 & 0x0Fu) << 4) | (px_1 & 0x0Fu));
}

////////////////////////////////////////////////////////////
// Write a single pixel byte (2 horizontal pixels) to the frame buffer
// Expects pointer to 2 Color values:
//   pixels[0] = even X pixel (low nibble)
//   pixels[1] = odd  X pixel (high nibble)
// x_byte and y are the coordinates of the left pixel,
//   expressed in byte address units along X.
////////////////////////////////////////////////////////////
void write_single_pixel_byte(const Color *restrict pixels, uint32_t y, uint32_t x_byte) {
    vga_write_single_pixel_byte_fast(pixels, y, x_byte);
}


////////////////////////////////////////////////////////////
// Write a 4x4 pixel block to the frame buffer
// Expects pointer to 16 Color values in row-major order:
//   block[row * 4 + col] for row,col in [0..3]
// x_byte and y are the coordinates of the top-left pixel,
//   expressed in byte address units along X.
////////////////////////////////////////////////////////////
void write_quad_4_pixels(const Color *restrict block, uint32_t y, uint32_t x_byte) {
    for (uint32_t row = 0; row < 4; ++row) {
        const Color *row_ptr = block + (row * 4u);
 
        uint16_t red_line = pack_four_pixels_16(
            row_ptr[0].r, row_ptr[1].r, row_ptr[2].r, row_ptr[3].r
        );
        uint16_t green_line = pack_four_pixels_16(
            row_ptr[0].g, row_ptr[1].g, row_ptr[2].g, row_ptr[3].g
        );
        uint16_t blue_line = pack_four_pixels_16(
            row_ptr[0].b, row_ptr[1].b, row_ptr[2].b, row_ptr[3].b
        );
 
        *(volatile uint16_t *)vga_color_addr_fast(VGA_RED_BASE,   y + row, x_byte) = red_line;
        *(volatile uint16_t *)vga_color_addr_fast(VGA_GREEN_BASE, y + row, x_byte) = green_line;
        *(volatile uint16_t *)vga_color_addr_fast(VGA_BLUE_BASE,  y + row, x_byte) = blue_line;
    }
}

////////////////////////////////////////////////////////////
// Write an 8x8 pixel block to the frame buffer
// Expects pointer to 64 Color values in row-major order:
//   block[row * 8 + col] for row,col in [0..7]
// x_byte and y are the coordinates of the top-left pixel,
//   expressed in byte address units along X.
////////////////////////////////////////////////////////////
void write_quad_8_pixels(const Color *restrict block, uint32_t y, uint32_t x_byte) {
    for (uint32_t row = 0; row < 8; ++row) {
        const Color *row_ptr = block + (row * 8u);
 
        uint32_t red_line = pack_eight_pixels_32(
            row_ptr[0].r, row_ptr[1].r, row_ptr[2].r, row_ptr[3].r,
            row_ptr[4].r, row_ptr[5].r, row_ptr[6].r, row_ptr[7].r
        );
        uint32_t green_line = pack_eight_pixels_32(
            row_ptr[0].g, row_ptr[1].g, row_ptr[2].g, row_ptr[3].g,
            row_ptr[4].g, row_ptr[5].g, row_ptr[6].g, row_ptr[7].g
        );
        uint32_t blue_line = pack_eight_pixels_32(
            row_ptr[0].b, row_ptr[1].b, row_ptr[2].b, row_ptr[3].b,
            row_ptr[4].b, row_ptr[5].b, row_ptr[6].b, row_ptr[7].b
        );
 
        *(volatile uint32_t *)vga_color_addr_fast(VGA_RED_BASE,   y + row, x_byte) = red_line;
        *(volatile uint32_t *)vga_color_addr_fast(VGA_GREEN_BASE, y + row, x_byte) = green_line;
        *(volatile uint32_t *)vga_color_addr_fast(VGA_BLUE_BASE,  y + row, x_byte) = blue_line;
    }
}

void write_rgb_byte(uint32_t y, uint32_t x_byte, uint8_t red_byte, uint8_t green_byte, uint8_t blue_byte) {
    vga_write_rgb_byte_fast(y, x_byte, red_byte, green_byte, blue_byte);
}

void write_rgb_row_bytes(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict green_row,
    const uint8_t *restrict blue_row,
    uint32_t count
) {
    vga_write_rgb_row_bytes_fast(y, red_row, green_row, blue_row, count);
}

void fill_rgb_row_constant(uint32_t y, uint8_t red_byte, uint8_t green_byte, uint8_t blue_byte, uint32_t count) {
    vga_fill_rgb_row_constant_fast(y, red_byte, green_byte, blue_byte, count);
}

void write_rgb_row_r_const_gb(
    uint32_t y,
    const uint8_t *restrict red_row,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    vga_write_rgb_row_r_const_gb_fast(y, red_row, green_byte, blue_byte, count);
}

void write_rgb_row_rb_const_g(
    uint32_t y,
    const uint8_t *restrict red_row,
    const uint8_t *restrict blue_row,
    uint8_t green_byte,
    uint32_t count
) {
    vga_write_rgb_row_rb_const_g_fast(y, red_row, blue_row, green_byte, count);
}

void write_rgb_column_bytes(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict green_col,
    const uint8_t *restrict blue_col,
    uint32_t count
) {
    vga_write_rgb_column_bytes_fast(y_start, x_byte, red_col, green_col, blue_col, count);
}

void fill_rgb_column_constant(
    uint32_t y_start,
    uint32_t x_byte,
    uint8_t red_byte,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    vga_fill_rgb_column_constant_fast(y_start, x_byte, red_byte, green_byte, blue_byte, count);
}

void write_rgb_column_r_const_gb(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    uint8_t green_byte,
    uint8_t blue_byte,
    uint32_t count
) {
    vga_write_rgb_column_r_const_gb_fast(y_start, x_byte, red_col, green_byte, blue_byte, count);
}

void write_rgb_column_rb_const_g(
    uint32_t y_start,
    uint32_t x_byte,
    const uint8_t *restrict red_col,
    const uint8_t *restrict blue_col,
    uint8_t green_byte,
    uint32_t count
) {
    vga_write_rgb_column_rb_const_g_fast(y_start, x_byte, red_col, blue_col, green_byte, count);
}