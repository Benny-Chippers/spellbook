#ifndef TETRIS_INPUT_H
#define TETRIS_INPUT_H

#include <stdint.h>

uint8_t tetris_input_update(void);
uint8_t tetris_input_left(void);
uint8_t tetris_input_right(void);
uint8_t tetris_input_rotate_cw(void);
uint8_t tetris_input_rotate_ccw(void);
uint8_t tetris_input_soft_drop(void);
uint8_t tetris_input_hard_drop(void);

#endif
