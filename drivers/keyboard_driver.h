#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stdint.h>

#include "delay.h"
#include "mmio_map.h"

#define KEYBOARD_BASE_ADDR SPI_SOUTHB_ADDR
#define KEYBOARD_KEY_COUNT 28u

#define KEYBOARD_KEY_A      0u
#define KEYBOARD_KEY_B      1u
#define KEYBOARD_KEY_C      2u
#define KEYBOARD_KEY_D      3u
#define KEYBOARD_KEY_E      4u
#define KEYBOARD_KEY_F      5u
#define KEYBOARD_KEY_G      6u
#define KEYBOARD_KEY_H      7u
#define KEYBOARD_KEY_I      8u
#define KEYBOARD_KEY_J      9u
#define KEYBOARD_KEY_K      10u
#define KEYBOARD_KEY_L      11u
#define KEYBOARD_KEY_M      12u
#define KEYBOARD_KEY_N      13u
#define KEYBOARD_KEY_O      14u
#define KEYBOARD_KEY_P      15u
#define KEYBOARD_KEY_Q      16u
#define KEYBOARD_KEY_R      17u
#define KEYBOARD_KEY_S      18u
#define KEYBOARD_KEY_T      19u
#define KEYBOARD_KEY_U      20u
#define KEYBOARD_KEY_V      21u
#define KEYBOARD_KEY_W      22u
#define KEYBOARD_KEY_X      23u
#define KEYBOARD_KEY_Y      24u
#define KEYBOARD_KEY_Z      25u
#define KEYBOARD_KEY_COMMA  26u
#define KEYBOARD_KEY_PERIOD 27u

static inline uint32_t keyboard_key_mask(uint8_t key) {
    if (key >= KEYBOARD_KEY_COUNT) {
        return 0u;
    }
    return (uint32_t)(1u << key);
}

static inline uint32_t keyboard_read_keys(void) {
    uint32_t keys = mmio_read32(KEYBOARD_BASE_ADDR);
    delay_us(15u);
    return keys;
}

static inline uint8_t keyboard_is_key_down(uint8_t key) {
    return (keyboard_read_keys() & keyboard_key_mask(key)) != 0u;
}

#endif
