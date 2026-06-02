#ifndef PONG_ASSETS_H
#define PONG_ASSETS_H

#include <stdint.h>

#define PONG_TRANSPARENT_INDEX 166u

#define PONG_BG_WIDTH  160u
#define PONG_BG_HEIGHT 120u
#define PONG_PADDLE_WIDTH  16u
#define PONG_PADDLE_HEIGHT 4u
#define PONG_BALL_WIDTH  2u
#define PONG_BALL_HEIGHT 2u

extern const uint8_t pong_palette_rgb888[768];
extern const uint8_t pong_background[];
extern const uint8_t pong_paddle[];
extern const uint8_t pong_ball[];

#endif
