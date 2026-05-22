/*
 * Linker script for RISC-V bare-metal test programs (Wizard Core map).
 * Source of truth: wizardCore/docs/memory_Map.md
 *
 * RAM: 128 KiB on-chip (32768 x 32-bit words), indexed by addr[16:2] in RTL.
 * VGA: write-only MMIO (stores only from CPU; loads are not defined). Symbol anchors
 * only — no ELF sections are placed in VGA regions.
 */

MEMORY
{
    RAM (rwx)       : ORIGIN = 0x00000000, LENGTH = 0x20000   /* 128 KiB */
    VGA_FB (w!r!a)  : ORIGIN = 0x10000000, LENGTH = 0x7800     /* indexed FB, WO */
    VGA_PAL (w!r!a) : ORIGIN = 0x10008000, LENGTH = 0x0300     /* palette R/G/B, WO */
    VGA_SWAP (w!r!a): ORIGIN = 0x10030000, LENGTH = 0x4         /* frame toggle, WO */
}

ENTRY(_start)

SECTIONS
{
    /* On-chip RAM geometry (also exported to C via mmio_map.h) */
    __ram_base = ORIGIN(RAM);
    __ram_size_bytes = LENGTH(RAM);
    __ram_size_words = LENGTH(RAM) / 4;
    __ram_last_addr = ORIGIN(RAM) + LENGTH(RAM) - 1;

    /* VGA write-only MMIO anchors — drivers store via MMIO_STORE*() in mmio_map.h */
    __vga_fb_base = ORIGIN(VGA_FB);
    __vga_palette_base = ORIGIN(VGA_PAL);
    __vga_pal_green = ORIGIN(VGA_PAL) + 0x100;
    __vga_pal_blue = ORIGIN(VGA_PAL) + 0x200;
    __vga_swap_addr = ORIGIN(VGA_SWAP);

    .text ORIGIN(RAM) : {
        *(.text.start)
        *(.text)
        *(.text.*)
        . = ALIGN(4);
    } > RAM

    .rodata : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > RAM

    .data : {
        *(.data)
        *(.data.*)
        . = ALIGN(4);
    } > RAM

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
    } > RAM

    _stack_start = .;
    . += 0x400;            /* 1 KiB stack */
    _stack_end = .;

    _heap_start = .;
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
}
