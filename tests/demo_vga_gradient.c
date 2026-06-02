#include <stdint.h>
#include "vga_driver.h"

/*
 * VGA demo: white (0xFFF) at top-left to blue (0x00F) at bottom-right.
 * Red ramps with X, green ramps with Y, blue stays at full brightness.
 * ~0.5 s between frame swaps (calibrated for ~5 MHz effective CPU).
 */

#define CPU_HZ 5000000u
#define DELAY_LOOP_CPI_EST 4u
#define HALF_SEC_ITERS (CPU_HZ / (2u * DELAY_LOOP_CPI_EST))

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

static void __attribute__((noinline)) draw_gradient_frame(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        uint8_t g = vga_gradient_g_equal_band_fast(y);
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            uint8_t r = vga_gradient_r_equal_band_fast(x);
            vga_write_index_fast(x, y, vga_palette_index_from_rg_fast(r, g));
        }
    }
}

int main(void) {
    vga_init_palette_rg_blue15_fast();

    draw_gradient_frame();
    swap_frame();
    draw_gradient_frame();
    swap_frame();
    delay_cycles(HALF_SEC_ITERS);

    for (;;) {
        draw_gradient_frame();
        swap_frame();
        delay_cycles(HALF_SEC_ITERS);
    }
}
