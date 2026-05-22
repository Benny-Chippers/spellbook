#include <stdint.h>
#include "vga_driver.h"

/* Tighter text layout for this demo only: 5x10 font, 1px char gap, 1px line gap. */
#define VGA_TEXT_CHAR_SPACING_X 1u
#define VGA_TEXT_LINE_SPACING_Y 1u
#include "vga_text.h"

#define CPU_HZ 5000000u
#define DELAY_LOOP_CPI_EST 4u
#define TWO_SEC_ITERS ((CPU_HZ * 2u) / DELAY_LOOP_CPI_EST)
#define TEN_SEC_ITERS ((CPU_HZ * 10u) / DELAY_LOOP_CPI_EST)

#define TEXT_MAX_VISIBLE_LINES (VGA_HEIGHT / VGA_TEXT_LINE_PITCH)
#define TEXT_MAX_CHARS_PER_LINE (VGA_WIDTH / VGA_TEXT_CELL_WIDTH)

static const char *const demo_lines[] = {
    "SPELLBOOK TEXT DEMO",
    "--------------------",
    "Wizard Core online.",
    "Init terminal...",
    "Glyph cache ready.",
    "Framebuffer RGB444",
    "Line pitch now 11 px",
    "Cell width now 6 px",
    "Writing script lines.",
    "Screen full -> scroll up",
    "Terminal style output.",
    "ASCII renderer active.",
    "No UART needed here.",
    "Swap trigger good.",
    "Render sequence done.",
    "Wizard book incoming..."
};

static const char *const wizard_book_art[] = {
    "C",
    " (\\.   \\      ,/)",
    "  \\(   |\\     )/",
    "  //\\  | \\   /\\\\",
    " (/ /\\_#oo#_/\\ \\)",
    "  \\/\\  ####  /\\/",
    "       `##'",
    "",
    " WIZARD CORE tm",
    "   POWERED BY FIREBALL"
};

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

static void clear_screen_black(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        vga_fill_row_rgb(y, 0u, 0u, 0u, VGA_WIDTH);
    }
}

static void draw_window(uint32_t written_count, Color fg, Color bg) {
    uint32_t start = 0u;
    uint32_t visible = written_count;
    uint32_t total_lines = (uint32_t)(sizeof(demo_lines) / sizeof(demo_lines[0]));

    if (written_count > TEXT_MAX_VISIBLE_LINES) {
        start = written_count - TEXT_MAX_VISIBLE_LINES;
        visible = TEXT_MAX_VISIBLE_LINES;
    }

    if (written_count > total_lines) {
        written_count = total_lines;
    }

    clear_screen_black();

    for (uint32_t i = 0; i < visible; ++i) {
        uint32_t src = start + i;
        if (src < written_count) {
            vga_text_draw_line(i, demo_lines[src], fg, bg, 0u);
        }
    }
}

static void draw_wizard_book(Color fg, Color bg) {
    uint32_t art_lines = (uint32_t)(sizeof(wizard_book_art) / sizeof(wizard_book_art[0]));
    for (uint32_t i = 0; i < art_lines && i < TEXT_MAX_VISIBLE_LINES; ++i) {
        vga_text_draw_line(i, wizard_book_art[i], fg, bg, 0u);
    }
}

int main(void) {
    const uint32_t total_lines = (uint32_t)(sizeof(demo_lines) / sizeof(demo_lines[0]));
    Color fg = {0x0u, 0xFu, 0x0u};
    Color bg = {0x0u, 0x0u, 0x0u};
    Color art_fg = {0xEu, 0xEu, 0x2u};

    vga_palette_reset();
    (void)TEXT_MAX_CHARS_PER_LINE;

    for (;;) {
        clear_screen_black();
        swap_frame();
        clear_screen_black();

        for (uint32_t written = 1u; written <= total_lines; ++written) {
            draw_window(written, fg, bg);
            swap_frame();
            delay_cycles(TWO_SEC_ITERS);
        }

        clear_screen_black();
        draw_wizard_book(art_fg, bg);
        swap_frame();
        delay_cycles(TEN_SEC_ITERS);
        clear_screen_black();
        swap_frame();
    }
}
