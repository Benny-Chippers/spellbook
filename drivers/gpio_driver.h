#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

#include "mmio_map.h"

#define GPIO_PIN(pin)             (1u << (pin))
#define GPIO_ALL_PINS             0xFFFFFFFFu
#define GPIO_CONFIGURABLE_PINS    GPIO_IO_ENABLE
#define GPIO_FORCED_OUTPUT_PINS   GPIO_FORCE_OUTPUT

/*
 * Wizard Core GPIO direction: 1 = output, 0 = input.
 * Pins outside GPIO_CONFIGURABLE_PINS are forced to output by hardware.
 */
static inline uint32_t gpio_read(void) {
    return mmio_read32(SPC_GPIO_DATA);
}

static inline void gpio_write(uint32_t value) {
    mmio_write32(SPC_GPIO_DATA, value);
}

static inline uint32_t gpio_read_direction(void) {
    return mmio_read32(SPC_GPIO_DIR);
}

static inline void gpio_write_direction(uint32_t output_mask) {
    mmio_write32(SPC_GPIO_DIR, output_mask);
}

static inline void gpio_configure_output(uint32_t pin_mask) {
    gpio_write_direction(gpio_read_direction() | pin_mask);
}

static inline void gpio_configure_input(uint32_t pin_mask) {
    gpio_write_direction(gpio_read_direction() & ~(pin_mask & GPIO_CONFIGURABLE_PINS));
}

static inline void gpio_write_masked(uint32_t pin_mask, uint32_t value) {
    uint32_t current = gpio_read();
    gpio_write((current & ~pin_mask) | (value & pin_mask));
}

static inline void gpio_set(uint32_t pin_mask) {
    gpio_write_masked(pin_mask, pin_mask);
}

static inline void gpio_clear(uint32_t pin_mask) {
    gpio_write_masked(pin_mask, 0u);
}

static inline uint32_t gpio_read_pin(uint32_t pin) {
    return (gpio_read() >> pin) & 1u;
}

static inline void gpio_write8(uint32_t byte_offset, uint8_t value) {
    mmio_write8(SPC_GPIO_DATA + (byte_offset & 3u), value);
}

static inline void gpio_write16(uint32_t halfword_offset, uint16_t value) {
    mmio_write16(SPC_GPIO_DATA + (halfword_offset & 2u), value);
}

#endif
