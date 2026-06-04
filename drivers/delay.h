#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

#include "mmio_map.h"

/*
 * Hardware-timed delays using Wizard Core's special-module 32-bit counter.
 * The counter increments every base clock cycle.
 *
 * DELAY_*_ITERS constants are kept for compatibility with older callers that
 * expressed time in units of the former addi/bnez software loop.
 */
#define DELAY_CPU_HZ           50000000u
#define DELAY_TIMER_HZ         DELAY_CPU_HZ
#define DELAY_CYCLES_PER_INSTR 5u
#define DELAY_CYCLES_PER_ITER  (2u * DELAY_CYCLES_PER_INSTR)
#define DELAY_ONE_SEC_ITERS    (DELAY_CPU_HZ / DELAY_CYCLES_PER_ITER)
/* RGB444 blue is 4-bit: 16 levels → one ramp step per 1/16 s for ~1 s rise or fall */
#define DELAY_BLUE_RAMP_STEPS  16u
#define DELAY_BLUE_STEP_ITERS  (DELAY_ONE_SEC_ITERS / DELAY_BLUE_RAMP_STEPS)

/* sp_gpio debounce: 2 ms at 50 MHz (wizardCore/src/COM/debounce.sv) */
#define DELAY_DEBOUNCE_MS      2u
#define DELAY_DEBOUNCE_ITERS   ((DELAY_ONE_SEC_ITERS * DELAY_DEBOUNCE_MS) / 1000u)

static inline uint32_t delay_counter_read(void) {
    return mmio_read32(SPC_COUNTER);
}

static inline void delay_counter_reset(void) {
    mmio_write32(SPC_COUNTER_RESET, 1u);
}

static inline void delay_cycles(uint32_t cycles) {
    uint32_t start;

    if (cycles == 0u) {
        return;
    }

    start = delay_counter_read();
    while ((uint32_t)(delay_counter_read() - start) < cycles) {
        __asm__ volatile ("nop" ::: "memory");
    }
}

static inline void delay_us(uint32_t us) {
    delay_cycles((DELAY_TIMER_HZ / 1000000u) * us);
}

static inline void delay_ms(uint32_t ms) {
    delay_cycles((DELAY_TIMER_HZ / 1000u) * ms);
}

static inline void delay_cpu_instructions(uint32_t iters) {
    if (iters == 0u) {
        return;
    }
    delay_cycles(iters * DELAY_CYCLES_PER_ITER);
}

#endif
