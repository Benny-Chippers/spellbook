#include "tetris_boot.h"

#include <stdint.h>

#include "delay.h"
#include "tetris_boot_assets.h"
#include "vga_blit.h"
#include "vga_driver.h"

#define TETRIS_BOOT_FADE_STEPS 32u
#define TETRIS_BOOT_FADE_STEP_CYCLES 3125000u
#define TETRIS_BOOT_HOLD_CYCLES 150000000u

static uint8_t __attribute__((noinline)) boot_scale_nibble(uint8_t value, uint8_t step) {
    volatile uint16_t sum = 16u;

    for (uint8_t i = 0u; i < step; ++i) {
        sum = (uint16_t)(sum + value);
    }

    return (uint8_t)(sum >> 5);
}

static void boot_load_faded_palette(uint8_t step) {
    for (uint8_t i = 0u; i < TETRIS_BOOT_ASSETS_PALETTE_COLORS; ++i) {
        const uint8_t *color = &tetris_boot_assets_palette_rgb444[(uint32_t)i * 3u];
        vga_palette_set_fast(
            i,
            boot_scale_nibble(color[0], step),
            boot_scale_nibble(color[1], step),
            boot_scale_nibble(color[2], step)
        );
    }
}

static void boot_present(void) {
    vga_present_index_buffer_fast(tetris_boot_assets_indices, TETRIS_BOOT_ASSETS_WIDTH);
    swap_frame();
}

void tetris_boot_show(void) {
    boot_load_faded_palette(0u);
    boot_present();
    boot_present();

    for (uint8_t step = 1u; step <= TETRIS_BOOT_FADE_STEPS; ++step) {
        uint32_t frame_start = delay_counter_read();
        uint32_t elapsed;

        boot_load_faded_palette(step);
        boot_present();

        elapsed = delay_counter_read() - frame_start;
        if (elapsed < TETRIS_BOOT_FADE_STEP_CYCLES) {
            delay_cycles(TETRIS_BOOT_FADE_STEP_CYCLES - elapsed);
        }
    }

    delay_cycles(TETRIS_BOOT_HOLD_CYCLES);
}
