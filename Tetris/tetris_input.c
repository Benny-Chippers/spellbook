#include "tetris_input.h"

#include "keyboard_driver.h"

#if defined(__GNUC__)
#define TETRIS_WEAK __attribute__((weak))
#else
#define TETRIS_WEAK
#endif

#define TETRIS_INPUT_POLL_FRAMES 6u

static uint32_t s_tetris_keys;
static uint8_t s_tetris_frames_until_poll;

TETRIS_WEAK uint8_t tetris_input_update(void) {
    if (s_tetris_frames_until_poll == 0u) {
        s_tetris_keys = keyboard_read_keys();
        s_tetris_frames_until_poll = (uint8_t)(TETRIS_INPUT_POLL_FRAMES - 1u);
        return 1u;
    }

    --s_tetris_frames_until_poll;
    return 0u;
}

static uint8_t tetris_input_key_down(uint8_t key) {
    return (s_tetris_keys & keyboard_key_mask(key)) != 0u;
}

TETRIS_WEAK uint8_t tetris_input_left(void) {
    return tetris_input_key_down(KEYBOARD_KEY_A);
}

TETRIS_WEAK uint8_t tetris_input_right(void) {
    return tetris_input_key_down(KEYBOARD_KEY_D);
}

TETRIS_WEAK uint8_t tetris_input_rotate_cw(void) {
    return tetris_input_key_down(KEYBOARD_KEY_Q);
}

TETRIS_WEAK uint8_t tetris_input_rotate_ccw(void) {
    return tetris_input_key_down(KEYBOARD_KEY_E);
}

TETRIS_WEAK uint8_t tetris_input_soft_drop(void) {
    return tetris_input_key_down(KEYBOARD_KEY_S);
}

TETRIS_WEAK uint8_t tetris_input_hard_drop(void) {
    return 0u;
}
