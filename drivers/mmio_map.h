#ifndef MMIO_MAP_H
#define MMIO_MAP_H

#include <stdint.h>

/*
 * Physical addresses exported by link.ld as absolute symbols.
 *
 * GNU ld assigns these with "= ORIGIN(...)" or "= constant". They are not
 * variables in RAM — the symbol *value* is the address. In C, declare each as
 * an extern array and use the decayed pointer:
 *
 *   extern char __vga_fb_base[];
 *   MMIO_STORE8(__vga_fb_base, offset, value);
 *
 * VGA regions are write-only from the CPU (link.ld: w!r!a). Do not load from
 * frame-buffer or palette addresses — RTL has no CPU read path for VGA stores.
 * Use MMIO_STORE*() for all VGA access so reads are never emitted by mistake.
 */

#define MMIO_ADDR(sym) ((uint32_t)(uintptr_t)(sym))

#define MMIO_STORE8(base, off, val) \
    do { *(volatile uint8_t *)(MMIO_ADDR(base) + (off)) = (uint8_t)(val); } while (0)

#define MMIO_STORE16(base, off, val) \
    do { *(volatile uint16_t *)(MMIO_ADDR(base) + (off)) = (uint16_t)(val); } while (0)

#define MMIO_STORE32(base, off, val) \
    do { *(volatile uint32_t *)(MMIO_ADDR(base) + (off)) = (uint32_t)(val); } while (0)

extern char __ram_base[];
extern char __ram_size_bytes[];
extern char __ram_size_words[];
extern char __ram_last_addr[];

extern char __vga_fb_base[];
extern char __vga_palette_base[];
extern char __vga_pal_green[];
extern char __vga_pal_blue[];
extern char __vga_swap_addr[];

#define RAM_BASE         MMIO_ADDR(__ram_base)
#define RAM_SIZE_BYTES   MMIO_ADDR(__ram_size_bytes)
#define RAM_SIZE_WORDS   MMIO_ADDR(__ram_size_words)
#define RAM_LAST_ADDR    MMIO_ADDR(__ram_last_addr)

#define VGA_FB_BASE      MMIO_ADDR(__vga_fb_base)
#define VGA_PALETTE_BASE MMIO_ADDR(__vga_palette_base)
#define VGA_PAL_RED      MMIO_ADDR(__vga_palette_base)
#define VGA_PAL_GREEN    MMIO_ADDR(__vga_pal_green)
#define VGA_PAL_BLUE     MMIO_ADDR(__vga_pal_blue)
#define VGA_SWAP_ADDR    MMIO_ADDR(__vga_swap_addr)

#endif
