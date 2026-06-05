#include "tetris_engine.h"

#include "delay.h"
#include "tetris_boot.h"

volatile int test_passed = 0;
volatile int test_failed = 0;
volatile int test_result = 0;

int main(void) {
    static tetris_game_t game;
    uint32_t last_frame_start;
    const uint32_t frame_cycles = DELAY_TIMER_HZ / 60u;

    tetris_boot_show();
    delay_counter_reset();

    tetris_init(&game);
    tetris_seed_vga_buffers(&game);

    tetris_render(&game);
    tetris_render(&game);
    last_frame_start = delay_counter_read();

    while (1) {
        uint32_t frame_start = delay_counter_read();
        uint32_t delta_cycles = frame_start - last_frame_start;
        uint32_t frame_elapsed;

        last_frame_start = frame_start;
        tetris_step(&game, delta_cycles);
        tetris_render(&game);

        frame_elapsed = delay_counter_read() - frame_start;
        if (frame_elapsed < frame_cycles) {
            delay_cycles(frame_cycles - frame_elapsed);
        }
    }

    return 0;
}
