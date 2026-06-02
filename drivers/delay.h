#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/*
 * Busy-wait calibrated for Wizard Core at 50 MHz, ~5 cycles per instruction.
 * Loop body is exactly addi + bnez (2 instructions → 10 cycles per iteration).
 */
#define DELAY_CPU_HZ           50000000u
#define DELAY_CYCLES_PER_INSTR 5u
#define DELAY_CYCLES_PER_ITER  (2u * DELAY_CYCLES_PER_INSTR)
#define DELAY_ONE_SEC_ITERS    (DELAY_CPU_HZ / DELAY_CYCLES_PER_ITER)
/* RGB444 blue is 4-bit: 16 levels → one ramp step per 1/16 s for ~1 s rise or fall */
#define DELAY_BLUE_RAMP_STEPS  16u
#define DELAY_BLUE_STEP_ITERS  (DELAY_ONE_SEC_ITERS / DELAY_BLUE_RAMP_STEPS)

/* sp_gpio debounce: 2 ms at 50 MHz (wizardCore/src/COM/debounce.sv) */
#define DELAY_DEBOUNCE_MS      2u
#define DELAY_DEBOUNCE_ITERS   ((DELAY_ONE_SEC_ITERS * DELAY_DEBOUNCE_MS) / 1000u)

static void __attribute__((noinline)) delay_cpu_instructions(uint32_t iters) {
    if (iters == 0u) {
        return;
    }
    __asm__ volatile (
        "1:\n"
        "  addi %0, %0, -1\n"
        "  bnez %0, 1b\n"
        : "+r"(iters)
        :
        : "memory"
    );
}

#endif
