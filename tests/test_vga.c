#include <stdint.h>
#include "vga_driver.h"

/*
 * VGA interface validation test for Wizard Core CPU.
 *
 * Address map: docs/vga/overview.md (wizardCore/docs/memory_Map.md).
 */

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

static void write_store_size_smoke_test(void) {
    volatile uint8_t *fb8 = (volatile uint8_t *)vga_fb_addr_fast(0u, 0u);
    volatile uint16_t *fb16 = (volatile uint16_t *)vga_fb_addr_fast(0u, 0u);
    volatile uint32_t *fb32 = (volatile uint32_t *)vga_fb_addr_fast(0u, 0u);

    *fb8 = 0xA5u;
    *fb16 = 0x5AA5u;
    *fb32 = 0x12345678u;

    ASSERT(1);
}

static void verify_coordinate_addressing(void) {
    uint32_t a00 = vga_fb_addr_fast(0u, 0u);
    uint32_t a01 = vga_fb_addr_fast(1u, 0u);
    uint32_t a10 = vga_fb_addr_fast(0u, 1u);

    ASSERT(a00 == VGA_FB_BASE);
    ASSERT((a01 - a00) == 1u);
    ASSERT((a10 - a00) == VGA_ROW_ADDR_STRIDE);
    ASSERT((vga_fb_addr_fast(159u, 119u) - VGA_FB_BASE) == 0x779Fu);
}

static void draw_gradient_frame(void) {
    vga_init_palette_rg_blue15_fast();
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        uint8_t g = (uint8_t)(15u - ((15u * y) / (VGA_HEIGHT - 1u)));
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            uint8_t r = (uint8_t)(15u - ((15u * x) / (VGA_WIDTH - 1u)));
            vga_write_index_fast(x, y, vga_palette_index_from_rg_fast(r, g));
        }
    }
}

static void draw_checker_frame(void) {
    vga_palette_reset();
    uint8_t light = vga_palette_alloc_color(0xEu, 0x1u, 0x1u);
    uint8_t dark = vga_palette_alloc_color(0x1u, 0xEu, 0xEu);

    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            uint8_t idx = ((x ^ y) & 1u) ? dark : light;
            vga_write_index_fast(x, y, idx);
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

    draw_gradient_frame();
    delay_cycles(1000u);

    swap_frame();
    draw_checker_frame();
    delay_cycles(1000u);

    for (;;) {
        swap_frame();
        delay_cycles(5000u);
    }
}
