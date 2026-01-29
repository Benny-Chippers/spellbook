/*
 * Linker script for RISC-V 32I bare metal test program
 * Adjust memory regions based on your FPGA memory map
 * 
 * Current configuration: 8KB total memory
 */

MEMORY
{
    /* Single unified 8KB RAM starting at 0x00000000.
     * For FPGA, map your instruction/data memory so that
     * address 0x00000000 is where the CPU fetches _start.
     */
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 8K
}

ENTRY(_start)

SECTIONS
{
    /* Code section (.text) - executable instructions, including bootloader */
    /* _start (from boot.S) is placed first in .text.start at address 0x00000000 */
    .text ORIGIN(RAM) : {
        *(.text.start)      /* Startup / bootloader code first */
        *(.text)            /* All other code */
        *(.text.*)          /* Code in sub-sections */
        . = ALIGN(4);       /* Align to 4-byte boundary */
    } > RAM
    
    /* Read-only data section (.rodata) - constants, strings */
    .rodata : {
        *(.rodata)         /* Read-only data */
        *(.rodata.*)       /* Read-only data in sub-sections */
        . = ALIGN(4);
    } > RAM
    
    /* Initialized data section (.data) - global/static variables with initial values */
    .data : {
        *(.data)           /* Initialized data */
        *(.data.*)         /* Initialized data in sub-sections */
        . = ALIGN(4);
    } > RAM
    
    /* Uninitialized data section (.bss) - global/static variables initialized to zero */
    .bss : {
        *(.bss)            /* Uninitialized data */
        *(.bss.*)          /* Uninitialized data in sub-sections */
        *(COMMON)          /* Common symbols */
        . = ALIGN(4);
    } > RAM
    
    /* Stack pointer initialization */
    /* Stack grows downward from _stack_end to _stack_start */
    _stack_start = .;      /* Bottom of stack (lowest address) */
    . += 0x400;            /* 1KB stack (0x400 = 1024 bytes) */
                            /* Adjust this value based on your needs:
                             * - Small programs: 0x200 (512 bytes)
                             * - Medium programs: 0x400 (1KB) 
                             * - Larger programs: 0x800 (2KB)
                             * Remember: stack + heap + code + data must fit in 8KB */
    _stack_end = .;        /* Top of stack (highest address) */
    
    /* Heap (if needed) - grows upward from _heap_start */
    _heap_start = .;
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);  /* End of RAM */
    
    /* Memory layout summary for 8KB (approximate):
     * - Bootloader + .text: from 0x00000000 upward
     * - .rodata/.data/.bss: following .text
     * - Stack: 1KB (adjustable)
     * - Heap: Remaining space up to 8KB
     */
}
