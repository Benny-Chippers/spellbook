#include "pong_engine.h"

#include "delay.h"

volatile int test_passed = 0;
volatile int test_failed = 0;
volatile int test_result = 0;

int main(void) {
    static pong_game_t game;

    pong_init(&game);
    pong_seed_vga_buffers(&game);

    delay_cpu_instructions(DELAY_ONE_SEC_ITERS / 2u);

    pong_start(&game);

    while (1) {
        pong_step(&game);
        pong_render(&game);
        delay_cpu_instructions(DELAY_ONE_SEC_ITERS / 60u);
    }

    return 0;
}
