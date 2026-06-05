#include "tetris_engine.h"

#include "tetris_input.h"
#include "vga_blit.h"
#include "vga_driver.h"

#define TETRIS_BOARD_X 62
#define TETRIS_BOARD_BOTTOM_Y 104
#define TETRIS_START_X 3
#define TETRIS_START_Y 19
#define TETRIS_START_DROP_MS 500u
#define TETRIS_MIN_DROP_MS 80u
#define TETRIS_START_DROP_CYCLES 25000000u
#define TETRIS_MIN_DROP_CYCLES 4000000u
#define TETRIS_SOFT_DROP_CYCLES 2500000u
#define TETRIS_MAX_FRAME_DELTA_CYCLES 5000000u
#define TETRIS_SPEEDUP_2_MS_CYCLES 100000u
#define TETRIS_SPEEDUP_4_MS_CYCLES 200000u
#define TETRIS_SPEEDUP_6_MS_CYCLES 300000u
#define TETRIS_SPEEDUP_8_MS_CYCLES 400000u

enum {
    TETRIS_SHAPE_T = 1u,
    TETRIS_SHAPE_I = 2u,
    TETRIS_SHAPE_O = 3u,
    TETRIS_SHAPE_S = 4u,
    TETRIS_SHAPE_Z = 5u,
    TETRIS_SHAPE_J = 6u,
    TETRIS_SHAPE_L = 7u,
};

static const uint8_t s_piece_t[4][4] = {
    {0u, 0u, 0u, 0u},
    {7u, 7u, 7u, 0u},
    {0u, 7u, 0u, 0u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_i[4][4] = {
    {0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u},
    {5u, 5u, 5u, 5u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_o[4][4] = {
    {0u, 0u, 0u, 0u},
    {0u, 3u, 3u, 0u},
    {0u, 3u, 3u, 0u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_s[4][4] = {
    {0u, 4u, 4u, 0u},
    {4u, 4u, 0u, 0u},
    {0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_z[4][4] = {
    {1u, 1u, 0u, 0u},
    {0u, 1u, 1u, 0u},
    {0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_j[4][4] = {
    {6u, 0u, 0u, 0u},
    {6u, 6u, 6u, 0u},
    {0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u},
};

static const uint8_t s_piece_l[4][4] = {
    {0u, 0u, 2u, 0u},
    {2u, 2u, 2u, 0u},
    {0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u},
};

static uint8_t tetris_rand8(tetris_game_t *game) {
    uint8_t x = game->prng;

    x ^= (uint8_t)(x << 3);
    x ^= (uint8_t)(x >> 5);
    x ^= (uint8_t)(x << 1);
    if (x == 0u) {
        x = 0x5Du;
    }

    game->prng = x;
    return x;
}

static uint8_t tetris_rand_bounded(tetris_game_t *game, uint8_t bound) {
    uint8_t value = tetris_rand8(game);

    while (value >= bound) {
        value = (uint8_t)(value - bound);
    }

    return value;
}

static void tetris_shuffle_bag(tetris_game_t *game) {
    for (uint8_t i = 0u; i < TETRIS_BAG_SIZE; ++i) {
        game->bag[i] = (uint8_t)(i + 1u);
    }

    for (int8_t i = (int8_t)TETRIS_BAG_SIZE - 1; i > 0; --i) {
        uint8_t j = tetris_rand_bounded(game, (uint8_t)(i + 1));
        uint8_t tmp = game->bag[(uint8_t)i];
        game->bag[(uint8_t)i] = game->bag[j];
        game->bag[j] = tmp;
    }

    game->bag_index = 0u;
}

static void tetris_copy_piece_grid(tetris_piece_t *piece, uint8_t size, const uint8_t src[4][4]) {
    piece->size = size;

    for (uint8_t y = 0u; y < 4u; ++y) {
        for (uint8_t x = 0u; x < 4u; ++x) {
            piece->grid[y][x] = y < size && x < size ? src[y][x] : 0u;
        }
    }
}

static void tetris_set_piece(tetris_piece_t *piece, uint8_t shape_id) {
    switch (shape_id) {
    case TETRIS_SHAPE_I:
        tetris_copy_piece_grid(piece, 4u, s_piece_i);
        break;
    case TETRIS_SHAPE_O:
        tetris_copy_piece_grid(piece, 4u, s_piece_o);
        break;
    case TETRIS_SHAPE_S:
        tetris_copy_piece_grid(piece, 3u, s_piece_s);
        break;
    case TETRIS_SHAPE_Z:
        tetris_copy_piece_grid(piece, 3u, s_piece_z);
        break;
    case TETRIS_SHAPE_J:
        tetris_copy_piece_grid(piece, 3u, s_piece_j);
        break;
    case TETRIS_SHAPE_L:
        tetris_copy_piece_grid(piece, 3u, s_piece_l);
        break;
    case TETRIS_SHAPE_T:
    default:
        tetris_copy_piece_grid(piece, 3u, s_piece_t);
        break;
    }
}

static uint8_t tetris_next_shape(tetris_game_t *game) {
    uint8_t shape;

    if (game->bag_index >= TETRIS_BAG_SIZE) {
        tetris_shuffle_bag(game);
    }

    shape = game->bag[game->bag_index];
    ++game->bag_index;
    return shape;
}

static void tetris_spawn(tetris_game_t *game) {
    game->piece.x = TETRIS_START_X;
    game->piece.y = TETRIS_START_Y;
    tetris_set_piece(&game->piece, tetris_next_shape(game));
}

static void tetris_clear_board(tetris_game_t *game) {
    for (uint8_t y = 0u; y < TETRIS_BOARD_HEIGHT; ++y) {
        game->board.collision[y] = 0u;
        for (uint8_t x = 0u; x < TETRIS_BOARD_WIDTH; ++x) {
            game->board.locations_colors[y][x] = 0u;
        }
    }
}

static uint8_t tetris_piece_collides(
    const tetris_game_t *game,
    int8_t target_x,
    int8_t target_y,
    const uint8_t grid[4][4],
    uint8_t size
) {
    for (uint8_t i = 0u; i < size; ++i) {
        for (uint8_t j = 0u; j < size; ++j) {
            int8_t row;
            int8_t col;

            if (grid[i][j] == 0u) {
                continue;
            }

            row = (int8_t)(target_y - (int8_t)size + (int8_t)i);
            col = (int8_t)(target_x + (int8_t)j);

            if (col < 0 || col >= (int8_t)TETRIS_BOARD_WIDTH || row < 0) {
                return 1u;
            }
            if (row >= (int8_t)TETRIS_BOARD_HEIGHT) {
                continue;
            }
            if ((game->board.collision[(uint8_t)row] & (uint16_t)(1u << (uint8_t)col)) != 0u) {
                return 1u;
            }
        }
    }

    return 0u;
}

static void tetris_rotate90(uint8_t size, uint8_t grid[4][4]) {
    for (uint8_t i = 0u; i < size / 2u; ++i) {
        for (uint8_t j = i; j < (uint8_t)(size - i - 1u); ++j) {
            uint8_t temp = grid[i][j];
            grid[i][j] = grid[size - 1u - j][i];
            grid[size - 1u - j][i] = grid[size - 1u - i][size - 1u - j];
            grid[size - 1u - i][size - 1u - j] = grid[j][size - 1u - i];
            grid[j][size - 1u - i] = temp;
        }
    }
}

static void tetris_rotate_piece(tetris_game_t *game, uint8_t rotations) {
    for (uint8_t i = 0u; i < rotations; ++i) {
        tetris_rotate90(game->piece.size, game->piece.grid);
    }

    if (tetris_piece_collides(game, game->piece.x, game->piece.y, game->piece.grid, game->piece.size) == 0u) {
        return;
    }
    if (tetris_piece_collides(game, (int8_t)(game->piece.x + 1), game->piece.y, game->piece.grid, game->piece.size) == 0u) {
        ++game->piece.x;
        return;
    }
    if (tetris_piece_collides(game, (int8_t)(game->piece.x - 1), game->piece.y, game->piece.grid, game->piece.size) == 0u) {
        --game->piece.x;
        return;
    }
    if (tetris_piece_collides(game, game->piece.x, (int8_t)(game->piece.y + 1), game->piece.grid, game->piece.size) == 0u) {
        ++game->piece.y;
        return;
    }

    for (uint8_t i = 0u; i < (uint8_t)(4u - rotations); ++i) {
        tetris_rotate90(game->piece.size, game->piece.grid);
    }
}

static void tetris_lock_piece(tetris_game_t *game) {
    for (uint8_t i = 0u; i < game->piece.size; ++i) {
        for (uint8_t j = 0u; j < game->piece.size; ++j) {
            int8_t row;
            int8_t col;
            uint8_t color = game->piece.grid[i][j];

            if (color == 0u) {
                continue;
            }

            row = (int8_t)(game->piece.y - (int8_t)game->piece.size + (int8_t)i);
            col = (int8_t)(game->piece.x + (int8_t)j);

            if (row >= 0 && row < (int8_t)TETRIS_BOARD_HEIGHT &&
                col >= 0 && col < (int8_t)TETRIS_BOARD_WIDTH) {
                game->board.collision[(uint8_t)row] |= (uint16_t)(1u << (uint8_t)col);
                game->board.locations_colors[(uint8_t)row][(uint8_t)col] = color;
            }
        }
    }
}

static uint8_t tetris_clear_full_lines(tetris_game_t *game) {
    uint8_t lines_cleared = 0u;

    for (uint8_t y = 0u; y < TETRIS_BOARD_HEIGHT; ++y) {
        if (game->board.collision[y] != 0x03FFu) {
            continue;
        }

        ++lines_cleared;
        for (uint8_t row = y; row < (uint8_t)(TETRIS_BOARD_HEIGHT - 1u); ++row) {
            game->board.collision[row] = game->board.collision[row + 1u];
            for (uint8_t x = 0u; x < TETRIS_BOARD_WIDTH; ++x) {
                game->board.locations_colors[row][x] = game->board.locations_colors[row + 1u][x];
            }
        }

        game->board.collision[TETRIS_BOARD_HEIGHT - 1u] = 0u;
        for (uint8_t x = 0u; x < TETRIS_BOARD_WIDTH; ++x) {
            game->board.locations_colors[TETRIS_BOARD_HEIGHT - 1u][x] = 0u;
        }

        if (y > 0u) {
            --y;
        }
    }

    return lines_cleared;
}

static void tetris_apply_line_score(tetris_game_t *game, uint8_t lines_cleared) {
    uint16_t speedup = 0u;
    uint32_t speedup_cycles = 0u;

    switch (lines_cleared) {
    case 1u:
        game->score = (uint16_t)(game->score + 100u);
        speedup = 2u;
        speedup_cycles = TETRIS_SPEEDUP_2_MS_CYCLES;
        break;
    case 2u:
        game->score = (uint16_t)(game->score + 300u);
        speedup = 4u;
        speedup_cycles = TETRIS_SPEEDUP_4_MS_CYCLES;
        break;
    case 3u:
        game->score = (uint16_t)(game->score + 500u);
        speedup = 6u;
        speedup_cycles = TETRIS_SPEEDUP_6_MS_CYCLES;
        break;
    case 4u:
        game->score = (uint16_t)(game->score + 800u);
        speedup = 8u;
        speedup_cycles = TETRIS_SPEEDUP_8_MS_CYCLES;
        break;
    default:
        break;
    }

    if (speedup != 0u) {
        if (game->drop_interval_ms > (uint16_t)(TETRIS_MIN_DROP_MS + speedup)) {
            game->drop_interval_ms = (uint16_t)(game->drop_interval_ms - speedup);
        } else {
            game->drop_interval_ms = TETRIS_MIN_DROP_MS;
        }
        if (game->drop_interval_cycles > TETRIS_MIN_DROP_CYCLES + speedup_cycles) {
            game->drop_interval_cycles -= speedup_cycles;
        } else {
            game->drop_interval_cycles = TETRIS_MIN_DROP_CYCLES;
        }
    }
}

static void tetris_lock_clear_and_spawn(tetris_game_t *game) {
    uint8_t lines_cleared;

    tetris_lock_piece(game);
    lines_cleared = tetris_clear_full_lines(game);
    tetris_apply_line_score(game, lines_cleared);
    tetris_spawn(game);

    if (tetris_piece_collides(game, game->piece.x, game->piece.y, game->piece.grid, game->piece.size) != 0u) {
        tetris_clear_board(game);
        game->score = 0u;
        game->drop_interval_ms = TETRIS_START_DROP_MS;
        game->drop_interval_cycles = TETRIS_START_DROP_CYCLES;
        game->drop_elapsed_cycles = 0u;
        tetris_shuffle_bag(game);
        tetris_spawn(game);
    }
}

static uint8_t tetris_move_down(tetris_game_t *game) {
    if (tetris_piece_collides(
            game,
            game->piece.x,
            (int8_t)(game->piece.y - 1),
            game->piece.grid,
            game->piece.size) != 0u) {
        tetris_lock_clear_and_spawn(game);
        return 0u;
    }

    --game->piece.y;
    return 1u;
}

static void tetris_blit_block(uint8_t *pixels, uint8_t block_id, int8_t board_x, int8_t board_row) {
    int32_t screen_y;

    if (block_id == 0u || block_id >= TETRIS_BLOCK_COUNT ||
        board_x < 0 || board_x >= (int8_t)TETRIS_BOARD_WIDTH ||
        board_row < 0 || board_row >= (int8_t)TETRIS_BOARD_HEIGHT) {
        return;
    }

    screen_y = TETRIS_BOARD_BOTTOM_Y - ((int32_t)board_row + 1) * (int32_t)TETRIS_TILE_SIZE;
    vga_index_buffer_blit_fast(
        pixels,
        TETRIS_BG_WIDTH,
        TETRIS_BG_HEIGHT,
        TETRIS_BOARD_X + (int32_t)board_x * (int32_t)TETRIS_TILE_SIZE,
        screen_y,
        TETRIS_TILE_SIZE,
        TETRIS_TILE_SIZE,
        tetris_block_pixels[block_id],
        TETRIS_TILE_SIZE,
        TETRIS_TRANSPARENT_INDEX
    );
}

void tetris_init(tetris_game_t *game) {
    tetris_clear_board(game);
    game->bag_index = TETRIS_BAG_SIZE;
    game->drop_elapsed_cycles = 0u;
    game->drop_interval_cycles = TETRIS_START_DROP_CYCLES;
    game->drop_interval_ms = TETRIS_START_DROP_MS;
    game->score = 0u;
    game->prng = 0xA7u;
    game->game_over = 0u;
    tetris_shuffle_bag(game);
    tetris_spawn(game);
}

void tetris_seed_vga_buffers(const tetris_game_t *game) {
    (void)game;
    vga_palette_load_rgb888_fast(tetris_palette_rgb888);
    swap_frame();
    vga_palette_load_rgb888_fast(tetris_palette_rgb888);
    swap_frame();
}

void tetris_step(tetris_game_t *game, uint32_t delta_cycles) {
    int8_t dx = 0;
    uint32_t active_drop_cycles;
    uint8_t input_updated = tetris_input_update();

    if (input_updated != 0u) {
        if (tetris_input_rotate_cw() != 0u) {
            tetris_rotate_piece(game, 1u);
        }
        if (tetris_input_rotate_ccw() != 0u) {
            tetris_rotate_piece(game, 3u);
        }

        if (tetris_input_left() != 0u) {
            dx = -1;
        }
        if (tetris_input_right() != 0u) {
            dx = 1;
        }
        if (dx != 0 &&
            tetris_piece_collides(game, (int8_t)(game->piece.x + dx), game->piece.y, game->piece.grid, game->piece.size) == 0u) {
            game->piece.x = (int8_t)(game->piece.x + dx);
        }

        if (tetris_input_hard_drop() != 0u) {
            while (tetris_piece_collides(game, game->piece.x, (int8_t)(game->piece.y - 1), game->piece.grid, game->piece.size) == 0u) {
                --game->piece.y;
            }
            tetris_lock_clear_and_spawn(game);
            game->drop_elapsed_cycles = 0u;
            return;
        }
    }

    active_drop_cycles = tetris_input_soft_drop() != 0u ? TETRIS_SOFT_DROP_CYCLES : game->drop_interval_cycles;
    if (delta_cycles > TETRIS_MAX_FRAME_DELTA_CYCLES) {
        delta_cycles = TETRIS_MAX_FRAME_DELTA_CYCLES;
    }

    if (UINT32_MAX - game->drop_elapsed_cycles < delta_cycles) {
        game->drop_elapsed_cycles = active_drop_cycles;
    } else {
        game->drop_elapsed_cycles += delta_cycles;
    }

    if (game->drop_elapsed_cycles >= active_drop_cycles) {
        if (tetris_move_down(game) != 0u) {
            game->drop_elapsed_cycles -= active_drop_cycles;
        } else {
            game->drop_elapsed_cycles = 0u;
        }
    }
}

void tetris_render(tetris_game_t *game) {
    vga_index_buffer_copy_fast(
        game->pixels,
        TETRIS_BG_WIDTH,
        TETRIS_BG_HEIGHT,
        tetris_background,
        TETRIS_BG_WIDTH,
        TETRIS_BG_HEIGHT
    );

    for (uint8_t row = 0u; row < TETRIS_BOARD_HEIGHT; ++row) {
        for (uint8_t x = 0u; x < TETRIS_BOARD_WIDTH; ++x) {
            tetris_blit_block(game->pixels, game->board.locations_colors[row][x], (int8_t)x, (int8_t)row);
        }
    }

    for (uint8_t y = 0u; y < game->piece.size; ++y) {
        for (uint8_t x = 0u; x < game->piece.size; ++x) {
            int8_t row;
            uint8_t block_id = game->piece.grid[y][x];

            if (block_id == 0u) {
                continue;
            }

            row = (int8_t)(game->piece.y - (int8_t)game->piece.size + (int8_t)y);
            tetris_blit_block(game->pixels, block_id, (int8_t)(game->piece.x + (int8_t)x), row);
        }
    }

    vga_present_index_buffer_fast(game->pixels, TETRIS_BG_WIDTH);
    swap_frame();
}
