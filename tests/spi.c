/*
 * Minimal SD-card SPI proof-of-concept.
 *
 * Build:
 *   make PROGRAM=spi
 */

#include <stdint.h>

#include "nb_driver.h"

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;
volatile uint32_t fail_addr = 0;
volatile uint32_t fail_expected = 0;
volatile uint32_t fail_actual = 0;

#define SD_SPI_BASE NB_SDCARD_BASE

static int check_sd_word(uint32_t offset, uint32_t expected) {
    uint32_t actual = nb_read32(SD_SPI_BASE, offset);

    if (actual == expected) {
        test_passed++;
        return 1;
    }

    test_failed++;
    test_result = 1u;
    fail_addr = nb_addr(SD_SPI_BASE, offset);
    fail_expected = expected;
    fail_actual = actual;
    return 0;
}

int main(void) {
    test_result = 0u;
    test_passed = 0u;
    test_failed = 0u;
    fail_addr = 0u;
    fail_expected = 0u;
    fail_actual = 0u;

    nb_write32(SD_SPI_BASE, 0x0124u, 0x13579BDFu);
    if (!check_sd_word(0x0124u, 0x13579BDFu)) {
        serial_write_cstr("sd spi fail\n");
        return 1;
    }

    nb_write32(SD_SPI_BASE, 0x04ACu, 0x2468ACE0u);
    if (!check_sd_word(0x04ACu, 0x2468ACE0u)) {
        serial_write_cstr("sd spi fail\n");
        return 1;
    }

    nb_write32(SD_SPI_BASE, 0x1234u, 0xA5C33C5Au);
    if (!check_sd_word(0x1234u, 0xA5C33C5Au)) {
        serial_write_cstr("sd spi fail\n");
        return 1;
    }

    serial_write_cstr("sd spi ok\n");
    return 0;
}
