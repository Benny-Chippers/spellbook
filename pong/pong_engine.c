#include "pong_engine.h"

#include "pong_assets.h"
#include "vga_blit.h"
#include "vga_driver.h"

static void pong_blit_sprite(
    uint8_t *dst,
    uint32_t dst_width,
    uint32_t dst_height,
    int32_t dx,
    int32_t dy,
    uint32_t sprite_width,
    uint32_t sprite_height,
    const uint8_t *src
) {
    vga_index_buffer_blit_fast(
        dst,
        dst_width,
        dst_height,
        dx,
        dy,
        sprite_width,
        sprite_height,
        src,
        sprite_width,
        PONG_TRANSPARENT_INDEX
    );
}

static void pong_draw_frame(pong_game_t *game) {
    vga_index_buffer_copy_fast(
        game->pixels,
        PONG_BG_WIDTH,
        PONG_BG_HEIGHT,
        pong_background,
        PONG_BG_WIDTH,
        PONG_BG_HEIGHT
    );
    pong_blit_sprite(
        game->pixels,
        PONG_BG_WIDTH,
        PONG_BG_HEIGHT,
        game->paddle1_x,
        game->paddle1_y,
        PONG_PADDLE_WIDTH,
        PONG_PADDLE_HEIGHT,
        pong_paddle
    );
    pong_blit_sprite(
        game->pixels,
        PONG_BG_WIDTH,
        PONG_BG_HEIGHT,
        game->paddle2_x,
        game->paddle2_y,
        PONG_PADDLE_WIDTH,
        PONG_PADDLE_HEIGHT,
        pong_paddle
    );
    pong_blit_sprite(
        game->pixels,
        PONG_BG_WIDTH,
        PONG_BG_HEIGHT,
        game->ball_x,
        game->ball_y,
        PONG_BALL_WIDTH,
        PONG_BALL_HEIGHT,
        pong_ball
    );
}

static int pong_overlap(
    uint8_t ax,
    uint8_t ay,
    uint8_t aw,
    uint8_t ah,
    uint8_t bx,
    uint8_t by,
    uint8_t bw,
    uint8_t bh
) {
    if ((int)ax + aw <= bx || ax >= (int)bx + bw) {
        return 0;
    }
    if ((int)ay + ah <= by || ay >= (int)by + bh) {
        return 0;
    }
    return 1;
}

void pong_init(pong_game_t *game) {
    game->paddle1_x = (uint8_t)(PONG_BG_WIDTH / 2u - PONG_PADDLE_WIDTH / 2u);
    game->paddle1_y = (uint8_t)(PONG_BG_HEIGHT / 6u);
    game->paddle2_x = (uint8_t)(PONG_BG_WIDTH / 2u - PONG_PADDLE_WIDTH / 2u);
    game->paddle2_y = (uint8_t)(PONG_BG_HEIGHT - PONG_BG_HEIGHT / 6u);
    game->ball_x = (uint8_t)(PONG_BG_WIDTH / 2u - PONG_BALL_WIDTH / 2u);
    game->ball_y = (uint8_t)(PONG_BG_HEIGHT / 2u - PONG_BALL_HEIGHT / 2u);
    game->ball_x_speed = 0;
    game->ball_y_speed = 0;
    game->started = 0u;
}

void pong_seed_vga_buffers(const pong_game_t *game) {
    (void)game;
    vga_palette_load_rgb888_fast(pong_palette_rgb888);
}

void pong_start(pong_game_t *game) {
    game->started = 1u;
    game->ball_y_speed = 1;
}

void pong_step(pong_game_t *game) {
    if (game->started == 0u) {
        game->ball_y_speed = 0;
        pong_start(game);
    }

    if (game->paddle1_x < game->ball_x + 6u) {
        game->paddle1_x = (uint8_t)(game->paddle1_x + 2u);
    }
    if (game->paddle1_x > game->ball_x - 6u) {
        game->paddle1_x = (uint8_t)(game->paddle1_x - 2u);
    }
    if (game->paddle2_x < game->ball_x + 6u) {
        game->paddle2_x = (uint8_t)(game->paddle2_x + 2u);
    }
    if (game->paddle2_x > game->ball_x - 6u) {
        game->paddle2_x = (uint8_t)(game->paddle2_x - 2u);
    }

    game->ball_y = (uint8_t)((int)game->ball_y + game->ball_y_speed);
    game->ball_x = (uint8_t)((int)game->ball_x + game->ball_x_speed);

    if (pong_overlap(
            game->paddle2_x,
            game->paddle2_y,
            PONG_PADDLE_WIDTH,
            PONG_PADDLE_HEIGHT,
            game->ball_x,
            game->ball_y,
            PONG_BALL_WIDTH,
            PONG_BALL_HEIGHT)) {
        if (game->ball_x >= game->paddle2_x && game->ball_x <= game->paddle2_x + 6u) {
            game->ball_y_speed = -1;
            game->ball_x_speed = -1;
        } else if (game->ball_x <= game->paddle2_x + PONG_PADDLE_WIDTH &&
                   game->ball_x >= game->paddle2_x + PONG_PADDLE_WIDTH - 6u) {
            game->ball_y_speed = -1;
            game->ball_x_speed = 1;
        } else {
            game->ball_y_speed = -1;
            game->ball_x_speed = 0;
        }
    }

    if (pong_overlap(
            game->paddle1_x,
            game->paddle1_y,
            PONG_PADDLE_WIDTH,
            PONG_PADDLE_HEIGHT,
            game->ball_x,
            game->ball_y,
            PONG_BALL_WIDTH,
            PONG_BALL_HEIGHT)) {
        if (game->ball_x >= game->paddle1_x && game->ball_x <= game->paddle1_x + 6u) {
            game->ball_y_speed = 1;
            game->ball_x_speed = -1;
        } else if (game->ball_x <= game->paddle1_x + PONG_PADDLE_WIDTH &&
                   game->ball_x >= game->paddle1_x + PONG_PADDLE_WIDTH - 6u) {
            game->ball_y_speed = 1;
            game->ball_x_speed = 1;
        } else {
            game->ball_y_speed = 1;
            game->ball_x_speed = 0;
        }
    }

    if (game->ball_x < 20u) {
        game->ball_x_speed = 1;
    }
    if (game->ball_x > 120u) {
        game->ball_x_speed = -1;
    }
    if (game->ball_y < 20u) {
        game->ball_x = (uint8_t)(PONG_BG_WIDTH / 2u - PONG_BALL_WIDTH / 2u);
        game->ball_y = (uint8_t)(PONG_BG_HEIGHT / 2u - PONG_BALL_HEIGHT / 2u);
        game->ball_x_speed = 0;
        game->ball_y_speed = -1;
        game->started = 0u;
    }
    if (game->ball_y > 100u) {
        game->ball_x = (uint8_t)(PONG_BG_WIDTH / 2u - PONG_BALL_WIDTH / 2u);
        game->ball_y = (uint8_t)(PONG_BG_HEIGHT / 2u - PONG_BALL_HEIGHT / 2u);
        game->ball_x_speed = 0;
        game->ball_y_speed = 1;
        game->started = 0u;
    }
}

void pong_render(pong_game_t *game) {
    pong_draw_frame(game);
    vga_present_index_buffer_fast(game->pixels, PONG_BG_WIDTH);
    swap_frame();
}
