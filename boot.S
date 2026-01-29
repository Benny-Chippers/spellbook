/* Simple bootloader for RV32I bare-metal CPU.
 * Sets stack pointer to _stack_end (from linker script)
 * and jumps to main().
 *
 * This code is linked at address 0x00000000 and ends up
 * in the first words of the ELF/BIN/HEX, so instruction
 * memory initialized from those files will include it
 * automatically (for FPGA or simulation).
 */

    .section .text.start
    .globl _start

_start:
    /* sp = &_stack_end */
    la      sp, _stack_end

    /* jump to main() */
    la      t0, main
    jr      t0

    /* If main ever returns, just spin here */
1:
    j       1b

