#include <stdint.h>
#include "vga_driver.h"
#include "vga_text.h"

#define CPU_HZ 5000000u
#define DELAY_LOOP_CPI_EST 4u
#define HOLD_ITERS ((CPU_HZ * 2u) / DELAY_LOOP_CPI_EST)

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

static void clear_screen_black(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        vga_fill_row_rgb(y, 0u, 0u, 0u, VGA_WIDTH);
    }
}

static void draw_charset(void) {
    const uint32_t cols = VGA_WIDTH / VGA_TEXT_CELL_WIDTH;
    const uint32_t max_rows = VGA_HEIGHT / VGA_TEXT_LINE_PITCH;
    const uint32_t total_slots = cols * max_rows;
    const uint8_t ascii_first = (uint8_t)VGA_TEXT_ASCII_FIRST;
    const uint8_t ascii_last = (uint8_t)VGA_TEXT_ASCII_LAST;
    Color fg = {0xFu, 0xFu, 0xFu};
    Color bg = {0x0u, 0x0u, 0x0u};
    uint32_t slot = 0u;

    for (uint8_t c = ascii_first; c <= ascii_last; ++c) {
        uint32_t row = slot / cols;
        uint32_t col = slot % cols;
        if (slot >= total_slots) {
            break;
        }
        vga_text_draw_ascii(vga_text_col_to_x(col), vga_text_line_to_y(row), c, fg, bg, 0u);
        ++slot;
    }
}

int main(void) {
    vga_palette_reset();
    clear_screen_black();
    draw_charset();
    swap_frame();

    for (;;) {
        delay_cycles(HOLD_ITERS);
    }
}
