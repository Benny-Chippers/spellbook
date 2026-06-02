#ifndef VGA_TEXT_H
#define VGA_TEXT_H

#include <stdint.h>
#include "vga_driver.h"

/*
 * Configurable text geometry defaults:
 * - 5x10 font box
 * - 2 px spacing between glyphs in a line (cell width = 7)
 * - 5 px spacing between lines (line pitch = 15)
 */
#ifndef VGA_TEXT_FONT_WIDTH
#define VGA_TEXT_FONT_WIDTH 5u
#endif

#ifndef VGA_TEXT_FONT_HEIGHT
#define VGA_TEXT_FONT_HEIGHT 10u
#endif

#ifndef VGA_TEXT_CHAR_SPACING_X
#define VGA_TEXT_CHAR_SPACING_X 2u
#endif

#ifndef VGA_TEXT_LINE_SPACING_Y
#define VGA_TEXT_LINE_SPACING_Y 5u
#endif

#define VGA_TEXT_CELL_WIDTH (VGA_TEXT_FONT_WIDTH + VGA_TEXT_CHAR_SPACING_X)
#define VGA_TEXT_LINE_PITCH (VGA_TEXT_FONT_HEIGHT + VGA_TEXT_LINE_SPACING_Y)

#define VGA_TEXT_ASCII_FIRST 32u
#define VGA_TEXT_ASCII_LAST  126u
#define VGA_TEXT_GLYPH_ROWS_5X7 7u

static const uint8_t vga_text_font_5x7[VGA_TEXT_ASCII_LAST - VGA_TEXT_ASCII_FIRST + 1u][VGA_TEXT_GLYPH_ROWS_5X7] = {
    [' ' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u},
    ['!' - VGA_TEXT_ASCII_FIRST] = {0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x00u, 0x04u},
    ['"' - VGA_TEXT_ASCII_FIRST] = {0x0Au, 0x0Au, 0x0Au, 0x00u, 0x00u, 0x00u, 0x00u},
    ['#' - VGA_TEXT_ASCII_FIRST] = {0x0Au, 0x0Au, 0x1Fu, 0x0Au, 0x1Fu, 0x0Au, 0x0Au},
    ['$' - VGA_TEXT_ASCII_FIRST] = {0x04u, 0x0Fu, 0x14u, 0x0Eu, 0x05u, 0x1Eu, 0x04u},
    ['%' - VGA_TEXT_ASCII_FIRST] = {0x19u, 0x19u, 0x02u, 0x04u, 0x08u, 0x13u, 0x13u},
    ['&' - VGA_TEXT_ASCII_FIRST] = {0x0Cu, 0x12u, 0x14u, 0x08u, 0x15u, 0x12u, 0x0Du},
    ['\'' - VGA_TEXT_ASCII_FIRST] = {0x06u, 0x04u, 0x08u, 0x00u, 0x00u, 0x00u, 0x00u},
    ['(' - VGA_TEXT_ASCII_FIRST] = {0x02u, 0x04u, 0x08u, 0x08u, 0x08u, 0x04u, 0x02u},
    [')' - VGA_TEXT_ASCII_FIRST] = {0x08u, 0x04u, 0x02u, 0x02u, 0x02u, 0x04u, 0x08u},
    ['*' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x04u, 0x15u, 0x0Eu, 0x15u, 0x04u, 0x00u},
    ['+' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x04u, 0x04u, 0x1Fu, 0x04u, 0x04u, 0x00u},
    [',' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x04u, 0x08u},
    ['-' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x00u, 0x1Fu, 0x00u, 0x00u, 0x00u},
    ['.' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x04u},
    ['/' - VGA_TEXT_ASCII_FIRST] = {0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x00u, 0x00u},
    ['0' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x13u, 0x15u, 0x19u, 0x11u, 0x0Eu},
    ['1' - VGA_TEXT_ASCII_FIRST] = {0x04u, 0x0Cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu},
    ['2' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1Fu},
    ['3' - VGA_TEXT_ASCII_FIRST] = {0x1Eu, 0x01u, 0x01u, 0x0Eu, 0x01u, 0x01u, 0x1Eu},
    ['4' - VGA_TEXT_ASCII_FIRST] = {0x02u, 0x06u, 0x0Au, 0x12u, 0x1Fu, 0x02u, 0x02u},
    ['5' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x01u, 0x01u, 0x1Eu},
    ['6' - VGA_TEXT_ASCII_FIRST] = {0x06u, 0x08u, 0x10u, 0x1Eu, 0x11u, 0x11u, 0x0Eu},
    ['7' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u},
    ['8' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x11u, 0x0Eu, 0x11u, 0x11u, 0x0Eu},
    ['9' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x11u, 0x0Fu, 0x01u, 0x02u, 0x0Cu},
    [':' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x04u, 0x00u, 0x00u, 0x04u, 0x00u, 0x00u},
    [';' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x04u, 0x00u, 0x00u, 0x04u, 0x04u, 0x08u},
    ['<' - VGA_TEXT_ASCII_FIRST] = {0x02u, 0x04u, 0x08u, 0x10u, 0x08u, 0x04u, 0x02u},
    ['=' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x1Fu, 0x00u, 0x1Fu, 0x00u, 0x00u},
    ['>' - VGA_TEXT_ASCII_FIRST] = {0x08u, 0x04u, 0x02u, 0x01u, 0x02u, 0x04u, 0x08u},
    ['?' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x00u, 0x04u},
    ['@' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x17u, 0x15u, 0x17u, 0x10u, 0x0Eu},
    ['A' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u},
    ['B' - VGA_TEXT_ASCII_FIRST] = {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x11u, 0x11u, 0x1Eu},
    ['C' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x10u, 0x10u, 0x10u, 0x11u, 0x0Eu},
    ['D' - VGA_TEXT_ASCII_FIRST] = {0x1Cu, 0x12u, 0x11u, 0x11u, 0x11u, 0x12u, 0x1Cu},
    ['E' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x1Fu},
    ['F' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x10u},
    ['G' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x10u, 0x17u, 0x11u, 0x11u, 0x0Fu},
    ['H' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u},
    ['I' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu},
    ['J' - VGA_TEXT_ASCII_FIRST] = {0x07u, 0x02u, 0x02u, 0x02u, 0x02u, 0x12u, 0x0Cu},
    ['K' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x12u, 0x14u, 0x18u, 0x14u, 0x12u, 0x11u},
    ['L' - VGA_TEXT_ASCII_FIRST] = {0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x1Fu},
    ['M' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x1Bu, 0x15u, 0x15u, 0x11u, 0x11u, 0x11u},
    ['N' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x19u, 0x15u, 0x13u, 0x11u, 0x11u},
    ['O' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu},
    ['P' - VGA_TEXT_ASCII_FIRST] = {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x10u, 0x10u, 0x10u},
    ['Q' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x11u, 0x11u, 0x11u, 0x15u, 0x12u, 0x0Du},
    ['R' - VGA_TEXT_ASCII_FIRST] = {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x14u, 0x12u, 0x11u},
    ['S' - VGA_TEXT_ASCII_FIRST] = {0x0Fu, 0x10u, 0x10u, 0x0Eu, 0x01u, 0x01u, 0x1Eu},
    ['T' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u},
    ['U' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu},
    ['V' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Au, 0x04u},
    ['W' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x11u, 0x15u, 0x15u, 0x15u, 0x0Au},
    ['X' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x0Au, 0x04u, 0x0Au, 0x11u, 0x11u},
    ['Y' - VGA_TEXT_ASCII_FIRST] = {0x11u, 0x11u, 0x0Au, 0x04u, 0x04u, 0x04u, 0x04u},
    ['Z' - VGA_TEXT_ASCII_FIRST] = {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x1Fu},
    ['[' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x08u, 0x08u, 0x08u, 0x08u, 0x08u, 0x0Eu},
    ['\\' - VGA_TEXT_ASCII_FIRST] = {0x10u, 0x08u, 0x04u, 0x02u, 0x01u, 0x00u, 0x00u},
    [']' - VGA_TEXT_ASCII_FIRST] = {0x0Eu, 0x02u, 0x02u, 0x02u, 0x02u, 0x02u, 0x0Eu},
    ['^' - VGA_TEXT_ASCII_FIRST] = {0x04u, 0x0Au, 0x11u, 0x00u, 0x00u, 0x00u, 0x00u},
    ['_' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Fu},
    ['`' - VGA_TEXT_ASCII_FIRST] = {0x08u, 0x04u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u},
    ['{' - VGA_TEXT_ASCII_FIRST] = {0x02u, 0x04u, 0x04u, 0x08u, 0x04u, 0x04u, 0x02u},
    ['|' - VGA_TEXT_ASCII_FIRST] = {0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u},
    ['}' - VGA_TEXT_ASCII_FIRST] = {0x08u, 0x04u, 0x04u, 0x02u, 0x04u, 0x04u, 0x08u},
    ['~' - VGA_TEXT_ASCII_FIRST] = {0x00u, 0x00u, 0x0Du, 0x12u, 0x00u, 0x00u, 0x00u},
};

/* Stretch 7 glyph rows into a 10-pixel-tall cell. */
static const uint8_t vga_text_row_map_10_from_7[VGA_TEXT_FONT_HEIGHT] = {
    0u, 1u, 1u, 2u, 3u, 3u, 4u, 5u, 5u, 6u
};

static inline uint32_t vga_text_col_to_x(uint32_t col) {
    return col * VGA_TEXT_CELL_WIDTH;
}

static inline uint32_t vga_text_line_to_y(uint32_t line) {
    return line * VGA_TEXT_LINE_PITCH;
}

static inline void vga_text_put_pixel(uint32_t x, uint32_t y, Color c) {
    vga_write_pixel_rgb(x, y, c.r, c.g, c.b);
}

static inline const uint8_t *vga_text_glyph_5x7(uint8_t ascii) {
    if (ascii >= 'a' && ascii <= 'z') {
        ascii = (uint8_t)(ascii - ('a' - 'A'));
    }

    if (ascii < VGA_TEXT_ASCII_FIRST || ascii > VGA_TEXT_ASCII_LAST) {
        ascii = '?';
    }

    {
        const uint8_t *glyph = vga_text_font_5x7[ascii - VGA_TEXT_ASCII_FIRST];
        uint8_t nonzero = 0u;
        if (ascii == ' ') {
            return glyph;
        }
        for (uint32_t i = 0; i < VGA_TEXT_GLYPH_ROWS_5X7; ++i) {
            nonzero = (uint8_t)(nonzero | glyph[i]);
        }
        if (nonzero == 0u) {
            return vga_text_font_5x7['?' - VGA_TEXT_ASCII_FIRST];
        }
        return glyph;
    }
}

static inline void vga_text_draw_ascii(
    uint32_t x,
    uint32_t y,
    uint8_t ascii,
    Color fg,
    Color bg,
    uint8_t draw_bg
) {
    const uint8_t *glyph = vga_text_glyph_5x7(ascii);

    for (uint32_t py = 0; py < VGA_TEXT_FONT_HEIGHT; ++py) {
        uint8_t row_bits = glyph[vga_text_row_map_10_from_7[py]];
        for (uint32_t px = 0; px < VGA_TEXT_FONT_WIDTH; ++px) {
            uint8_t bit = (uint8_t)((row_bits >> (VGA_TEXT_FONT_WIDTH - 1u - px)) & 1u);
            if (bit) {
                vga_text_put_pixel(x + px, y + py, fg);
            } else if (draw_bg) {
                vga_text_put_pixel(x + px, y + py, bg);
            }
        }
    }

    if (draw_bg) {
        for (uint32_t py = 0; py < VGA_TEXT_FONT_HEIGHT; ++py) {
            for (uint32_t px = VGA_TEXT_FONT_WIDTH; px < VGA_TEXT_CELL_WIDTH; ++px) {
                vga_text_put_pixel(x + px, y + py, bg);
            }
        }
        for (uint32_t py = VGA_TEXT_FONT_HEIGHT; py < VGA_TEXT_LINE_PITCH; ++py) {
            for (uint32_t px = 0; px < VGA_TEXT_CELL_WIDTH; ++px) {
                vga_text_put_pixel(x + px, y + py, bg);
            }
        }
    }
}

static inline void vga_text_draw_ascii_span(
    uint32_t x,
    uint32_t y,
    const uint8_t *ascii_codes,
    uint32_t count,
    Color fg,
    Color bg,
    uint8_t draw_bg
) {
    for (uint32_t i = 0; i < count; ++i) {
        vga_text_draw_ascii(x + (i * VGA_TEXT_CELL_WIDTH), y, ascii_codes[i], fg, bg, draw_bg);
    }
}

static inline void vga_text_draw_cstr(
    uint32_t x,
    uint32_t y,
    const char *text,
    Color fg,
    Color bg,
    uint8_t draw_bg
) {
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;

    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            cursor_y += VGA_TEXT_LINE_PITCH;
        } else {
            vga_text_draw_ascii(cursor_x, cursor_y, (uint8_t)*text, fg, bg, draw_bg);
            cursor_x += VGA_TEXT_CELL_WIDTH;
        }
        ++text;
    }
}

static inline void vga_text_draw_line(
    uint32_t line_index,
    const char *text,
    Color fg,
    Color bg,
    uint8_t draw_bg
) {
    vga_text_draw_cstr(0u, vga_text_line_to_y(line_index), text, fg, bg, draw_bg);
}

#endif
