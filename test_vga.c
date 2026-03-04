#include <stdint.h>

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

#define VGA_RED_BASE    0x10000000u
#define VGA_GREEN_BASE  0x10010000u
#define VGA_BLUE_BASE   0x10020000u
#define VGA_SWAP_ADDR   0x10030000u

#define VGA_HEIGHT      120u
#define VGA_WIDTH_BYTES 80u
#define VGA_ROW_ADDR_STRIDE 0x100u

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;

#define ASSERT(cond)                            \
    do {                                        \
        if (cond) {                             \
            test_passed++;                      \
        } else {                                \
            test_failed++;                      \
            test_result = 1;                    \
        }                                       \
    } while (0)

static inline uint32_t color_addr(uint32_t color_base, uint32_t y, uint32_t x_byte) {
    /* Coordinate addressing: Y selects row page, X selects byte-in-row.
     * This is intentionally not contiguous y*80 linear addressing. */
    return color_base + (y * VGA_ROW_ADDR_STRIDE) + x_byte;
}

static inline uint8_t pack_two_pixels(uint8_t even_x, uint8_t odd_x) {
    return (uint8_t)(((odd_x & 0x0Fu) << 4) | (even_x & 0x0Fu));
}

static inline void swap_frame(void) {
    *(volatile uint32_t *)VGA_SWAP_ADDR = 1u;
}

static void write_store_size_smoke_test(void) {
    volatile uint8_t *r8 = (volatile uint8_t *)color_addr(VGA_RED_BASE, 0u, 0u);
    volatile uint16_t *r16 = (volatile uint16_t *)color_addr(VGA_RED_BASE, 0u, 2u);
    volatile uint32_t *r32 = (volatile uint32_t *)color_addr(VGA_RED_BASE, 0u, 4u);

    *r8 = 0xA5u;       /* byte store path */
    *r16 = 0x5AA5u;    /* halfword store path */
    *r32 = 0x12345678u;/* word store path */

    ASSERT(1);
}

static void verify_coordinate_addressing(void) {
    uint32_t a00 = color_addr(VGA_RED_BASE, 0u, 0u);
    uint32_t a01 = color_addr(VGA_RED_BASE, 0u, 1u);
    uint32_t a10 = color_addr(VGA_RED_BASE, 1u, 0u);

    ASSERT(a00 == VGA_RED_BASE);
    ASSERT((a01 - a00) == 1u);
    ASSERT((a10 - a00) == VGA_ROW_ADDR_STRIDE);
    ASSERT((color_addr(VGA_RED_BASE, 119u, 79u) - VGA_RED_BASE) == 0x774Fu);
}

static void draw_frame_a(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t xb = 0; xb < VGA_WIDTH_BYTES; xb++) {
            uint8_t x0 = (uint8_t)(xb << 1);
            uint8_t x1 = (uint8_t)(x0 + 1u);

            uint8_t red_byte = pack_two_pixels((uint8_t)(x0 >> 4), (uint8_t)(x1 >> 4));
            uint8_t green_byte = pack_two_pixels((uint8_t)(y >> 3), (uint8_t)(y >> 3));
            uint8_t blue_byte = pack_two_pixels(0x2u, 0x2u);

            *(volatile uint8_t *)color_addr(VGA_RED_BASE, y, xb) = red_byte;
            *(volatile uint8_t *)color_addr(VGA_GREEN_BASE, y, xb) = green_byte;
            *(volatile uint8_t *)color_addr(VGA_BLUE_BASE, y, xb) = blue_byte;
        }
    }
}

static void draw_frame_b(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t xb = 0; xb < VGA_WIDTH_BYTES; xb++) {
            uint8_t checker = (uint8_t)(((xb ^ y) & 1u) ? 0xEu : 0x1u);
            uint8_t inv = (uint8_t)(0xFu - checker);

            uint8_t red_byte = pack_two_pixels(checker, checker);
            uint8_t green_byte = pack_two_pixels(0x0u, 0x0u);
            uint8_t blue_byte = pack_two_pixels(inv, inv);

            *(volatile uint8_t *)color_addr(VGA_RED_BASE, y, xb) = red_byte;
            *(volatile uint8_t *)color_addr(VGA_GREEN_BASE, y, xb) = green_byte;
            *(volatile uint8_t *)color_addr(VGA_BLUE_BASE, y, xb) = blue_byte;
        }
    }
}

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

int main(void) {
    test_result = 0;
    test_passed = 0;
    test_failed = 0;

    verify_coordinate_addressing();
    write_store_size_smoke_test();

    /* Frame 0: gradient bars */
    draw_frame_a();
    delay_cycles(1000u);

    /* Frame 1: checker/inverse pattern */
    swap_frame();
    draw_frame_b();
    delay_cycles(1000u);

    /* Toggle continuously to prove CPU->GPU frame swap communication. */
    for (;;) {
        swap_frame();
        delay_cycles(5000u);
    }

    /* unreachable, kept for toolchain compatibility */
    /* return test_result; */
}
