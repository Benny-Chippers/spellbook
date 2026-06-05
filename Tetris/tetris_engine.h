#ifndef TETRIS_ENGINE_H
#define TETRIS_ENGINE_H

#include <stdint.h>

#include "tetris_assets.h"

#define TETRIS_BOARD_WIDTH  10u
#define TETRIS_BOARD_HEIGHT 20u
#define TETRIS_BAG_SIZE     7u

typedef struct tetris_piece {
    uint8_t size;
    int8_t x;
    int8_t y;
    uint8_t grid[4][4];
} tetris_piece_t;

typedef struct tetris_board {
    uint16_t collision[TETRIS_BOARD_HEIGHT];
    uint8_t locations_colors[TETRIS_BOARD_HEIGHT][TETRIS_BOARD_WIDTH];
} tetris_board_t;

typedef struct tetris_game {
    uint8_t pixels[TETRIS_BG_WIDTH * TETRIS_BG_HEIGHT];
    tetris_board_t board;
    tetris_piece_t piece;
    uint8_t bag[TETRIS_BAG_SIZE];
    uint8_t bag_index;
    uint32_t drop_elapsed_cycles;
    uint32_t drop_interval_cycles;
    uint16_t drop_interval_ms;
    uint16_t score;
    uint8_t prng;
    uint8_t game_over;
} tetris_game_t;

void tetris_init(tetris_game_t *game);
void tetris_seed_vga_buffers(const tetris_game_t *game);
void tetris_step(tetris_game_t *game, uint32_t delta_cycles);
void tetris_render(tetris_game_t *game);

#endif
