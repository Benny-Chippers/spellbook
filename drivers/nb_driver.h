#ifndef NB_DRIVER_H
#define NB_DRIVER_H

#include <stdint.h>

#include "mmio_map.h"

#define NB_PSRAM_BASE    SPI_PSRAM_ADDR
#define NB_FLASH_BASE    SPI_FLASH_ADDR
#define NB_SDCARD_BASE   SPI_SDCARD_ADDR
#define NB_SOUTHB_BASE   SPI_SOUTHB_ADDR
#define NB_SERIAL_BASE   SPI_SERIAL_ADDR

/* Backward-compatible names for older tests. */
#define PSRAM_ADDR       NB_PSRAM_BASE
#define FLASH_ADDR       NB_FLASH_BASE
#define SD_ADDR          NB_SDCARD_BASE
#define SB_ADDR          NB_SOUTHB_BASE
#define SERIAL_ADDR      NB_SERIAL_BASE

static inline uint32_t nb_addr(uint32_t base, uint32_t offset) {
    return base + offset;
}

static inline void nb_write8(uint32_t base, uint32_t offset, uint8_t value) {
    mmio_write8(nb_addr(base, offset), value);
}

static inline void nb_write16(uint32_t base, uint32_t offset, uint16_t value) {
    mmio_write16(nb_addr(base, offset), value);
}

static inline void nb_write32(uint32_t base, uint32_t offset, uint32_t value) {
    mmio_write32(nb_addr(base, offset), value);
}

static inline uint8_t nb_read8(uint32_t base, uint32_t offset) {
    return mmio_read8(nb_addr(base, offset));
}

static inline uint16_t nb_read16(uint32_t base, uint32_t offset) {
    return mmio_read16(nb_addr(base, offset));
}

static inline uint32_t nb_read32(uint32_t base, uint32_t offset) {
    return mmio_read32(nb_addr(base, offset));
}

static inline void serial_write_byte(uint8_t value) {
    mmio_write8(NB_SERIAL_BASE, value);
}

static inline void serial_write(const char *str, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        serial_write_byte((uint8_t)str[i]);
    }
}

static inline void serial_write_cstr(const char *str) {
    while (*str != '\0') {
        serial_write_byte((uint8_t)*str);
        str++;
    }
}

#endif
