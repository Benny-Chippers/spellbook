#ifndef MMIO_MAP_H
#define MMIO_MAP_H

#include <stdint.h>

/*
 * Linker absolute symbols (link.ld). MMIO_ADDR() yields the physical address.
 * Stores use inline asm so GCC never emits read-modify-write byte sequences
 * on write-only VGA (a 32-bit volatile store can become lbu/sb pairs and
 * corrupt adjacent frame-buffer bytes, e.g. at 0x1003_0000).
 */

#define MMIO_ADDR(sym) ((uint32_t)(uintptr_t)(sym))

static inline void mmio_write8(uint32_t addr, uint8_t val) {
    __asm__ volatile ("sb %0, 0(%1)" : : "r"(val), "r"(addr) : "memory");
}

static inline void mmio_write16(uint32_t addr, uint16_t val) {
    __asm__ volatile ("sh %0, 0(%1)" : : "r"(val), "r"(addr) : "memory");
}

static inline void mmio_write32(uint32_t addr, uint32_t val) {
    __asm__ volatile ("sw %0, 0(%1)" : : "r"(val), "r"(addr) : "memory");
}

static inline uint8_t mmio_read8(uint32_t addr) {
    uint32_t val;
    __asm__ volatile ("lbu %0, 0(%1)" : "=r"(val) : "r"(addr) : "memory");
    return (uint8_t)val;
}

static inline uint16_t mmio_read16(uint32_t addr) {
    uint32_t val;
    __asm__ volatile ("lhu %0, 0(%1)" : "=r"(val) : "r"(addr) : "memory");
    return (uint16_t)val;
}

#define MMIO_STORE8(base, off, val) \
    mmio_write8(MMIO_ADDR(base) + (uint32_t)(off), (uint8_t)(val))

#define MMIO_STORE16(base, off, val) \
    mmio_write16(MMIO_ADDR(base) + (uint32_t)(off), (uint16_t)(val))

#define MMIO_STORE32(base, off, val) \
    mmio_write32(MMIO_ADDR(base) + (uint32_t)(off), (uint32_t)(val))

extern char __ram_base[];
extern char __ram_size_bytes[];
extern char __ram_size_words[];
extern char __ram_last_addr[];

extern char __vga_fb_base[];
extern char __vga_palette_base[];
extern char __vga_pal_green[];
extern char __vga_pal_blue[];
extern char __vga_swap_addr[];

extern char __spi_psram_base[];
extern char __spi_flash_base[];
extern char __spi_sdcard_base[];
extern char __spi_southb_base[];
extern char __spi_serial_base[];

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

#define SPI_PSRAM_ADDR     MMIO_ADDR(__spi_psram_base)
#define SPI_FLASH_ADDR     MMIO_ADDR(__spi_flash_base)
#define SPI_SDCARD_ADDR    MMIO_ADDR(__spi_sdcard_base)
#define SPI_SOUTHB_ADDR    MMIO_ADDR(__spi_southb_base)
#define SPI_SERIAL_ADDR    MMIO_ADDR(__spi_serial_base)

/* Special module registers (addr[29:28] == 0b11, wizardCore/docs/memory_Map.md) */
#define SPC_GPIO_DATA         0x30000000u
#define SPC_GPIO_DIR          0x30000004u
#define SPC_COUNTER           0x30010000u
#define SPC_COUNTER_RESET     0x30011000u

/*
 * sp_gpio.sv IO_ENABLE — 1 = software may set direction; 0 = forced output.
 * Matches 32'b1111_1111_1000_0000_1111_1111_1000_0000.
 */
#define GPIO_IO_ENABLE        0xFF80FF80u
#define GPIO_FORCE_OUTPUT     (~GPIO_IO_ENABLE)

static inline uint32_t mmio_read32(uint32_t addr) {
    uint32_t val;
    __asm__ volatile ("lw %0, 0(%1)" : "=r"(val) : "r"(addr) : "memory");
    return val;
}

#endif
