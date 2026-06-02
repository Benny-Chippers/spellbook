/*
 * Special module register test (Wizard Core memory map).
 *
 * Exercises IO Port (GPIO data + direction) and the 32-bit timing counter.
 * Map: wizardCore/docs/memory_Map.md
 *
 * Build (rv32i — Makefile forces RV32I_ONLY for Verilator sim):
 *   make PROGRAM=test_special_regs
 *
 * Simulation:
 *   cp test_special_regs.mem ../wizardCore/scripts/
 *   cd ../wizardCore && make simview
 *
 * Drive debounced input pin 8 from the testbench (docs/special_regs/testbench_gpio.sv).
 */

#include <stdint.h>
#include "delay.h"
#include "mmio_map.h"

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;

#define ASSERT(cond) \
    do { \
        if (cond) { \
            test_passed++; \
        } else { \
            test_failed++; \
            test_result = 1; \
        } \
    } while (0)

#define GPIO_DATA  ((volatile uint32_t *)SPC_GPIO_DATA)
#define GPIO_DIR   ((volatile uint32_t *)SPC_GPIO_DIR)
#define COUNTER    ((volatile uint32_t *)SPC_COUNTER)
#define CNT_RESET  ((volatile uint32_t *)SPC_COUNTER_RESET)

static void test_gpio_forced_output_latch(void) {
    const uint32_t pattern = 0xA5A5A5A5u;

    *GPIO_DATA = pattern;
    ASSERT((*GPIO_DATA & GPIO_FORCE_OUTPUT) == (pattern & GPIO_FORCE_OUTPUT));
}

static void test_gpio_direction_mask(void) {
    *GPIO_DIR = 0x00000000u;
    ASSERT((*GPIO_DIR & GPIO_FORCE_OUTPUT) == GPIO_FORCE_OUTPUT);
    ASSERT((*GPIO_DIR & GPIO_IO_ENABLE) == 0u);
}

static void test_gpio_byte_store(void) {
    volatile uint8_t *data8 = (volatile uint8_t *)SPC_GPIO_DATA;

    *GPIO_DIR = 0xFFFFFFFFu;
    *GPIO_DATA = 0x00000000u;
    data8[0] = 0x5Au;
    data8[2] = 0xC3u;
    ASSERT((*GPIO_DATA & 0x00FF00FFu) == 0x00C3005Au);
}

static void test_gpio_input_pin8(void) {
    const uint32_t pin8 = 1u << 8;

    *GPIO_DIR = ~pin8;
    ASSERT((*GPIO_DIR & pin8) == 0u);

    /*
     * Debounce filter is 2 ms (debounce.sv). Wait before sampling the pin.
     * Testbench drives gpio[8] high (docs/special_regs/testbench_gpio.sv).
     */
    delay_cpu_instructions(DELAY_DEBOUNCE_ITERS * 2u);
    ASSERT((*GPIO_DATA & pin8) == pin8);
}

static void test_timing_counter(void) {
    uint32_t start;
    uint32_t mid;
    uint32_t after_reset;

    *CNT_RESET = 1u;
    start = *COUNTER;

    delay_cpu_instructions(DELAY_ONE_SEC_ITERS / 1000u);
    mid = *COUNTER;
    ASSERT(mid > start);

    *CNT_RESET = 1u;
    after_reset = *COUNTER;
    ASSERT(after_reset <= mid);
}

int main(void) {
    test_result = 0;
    test_passed = 0;
    test_failed = 0;

    test_gpio_forced_output_latch();
    test_gpio_direction_mask();
    test_gpio_byte_store();
    test_gpio_input_pin8();
    test_timing_counter();

    return test_result;
}
