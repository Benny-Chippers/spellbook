#ifndef PONG_ENGINE_H
#define PONG_ENGINE_H

#include <stdint.h>

#include "pong_assets.h"

typedef struct pong_game {
    uint8_t pixels[PONG_BG_WIDTH * PONG_BG_HEIGHT];
    uint8_t paddle1_x;
    uint8_t paddle1_y;
    uint8_t paddle2_x;
    uint8_t paddle2_y;
    uint8_t ball_x;
    uint8_t ball_y;
    int8_t ball_x_speed;
    int8_t ball_y_speed;
    uint8_t started;
} pong_game_t;

void pong_init(pong_game_t *game);
void pong_seed_vga_buffers(const pong_game_t *game);
void pong_start(pong_game_t *game);
void pong_step(pong_game_t *game);
void pong_render(pong_game_t *game);

#endif
