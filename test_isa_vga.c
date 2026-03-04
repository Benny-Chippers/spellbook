/*
 * Combined RV32I + VGA regression test.
 *
 * Runs ISA coverage checks, then exercises VGA memory-mapped interface by:
 *  - store-size smoke writes (SB/SH/SW)
 *  - drawing two full frames
 *  - verifying sampled bytes from each color plane
 *  - triggering frame swaps
 *
 * Returns 0 on pass, 1 on first failure.
 */

#include <stdint.h>

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;
volatile uint32_t fail_code = 0;
volatile uint32_t vga_stage = 0;

#define ASSERT(condition, code) \
    do { \
        if (condition) { \
            test_passed++; \
        } else { \
            test_failed++; \
            test_result = 1; \
            fail_code = (code); \
            return; \
        } \
    } while (0)

/* ---------- ISA tests ---------- */

static void test_arithmetic(void) {
    volatile int32_t a = 10;
    volatile int32_t b = 5;
    volatile int32_t result;

    result = a + b;
    ASSERT(result == 15, 1);

    result = a - b;
    ASSERT(result == 5, 2);

    result = a & b;
    ASSERT(result == 0, 3);

    result = a | b;
    ASSERT(result == 15, 4);

    result = a ^ b;
    ASSERT(result == 15, 5);

    result = a << 2;
    ASSERT(result == 40, 6);

    result = (uint32_t)a >> 2;
    ASSERT(result == 2, 7);

    volatile uint32_t shift_amt = 3;
    result = (uint32_t)(a << 3) >> shift_amt;
    ASSERT(result == 10, 8);

    volatile int32_t neg = -16;
    result = neg >> 2;
    ASSERT(result == -4, 9);

    result = (a < b) ? 1 : 0;
    ASSERT(result == 0, 10);
    result = (b < a) ? 1 : 0;
    ASSERT(result == 1, 11);

    volatile uint32_t ua = 0xFFFFFFFFu;
    volatile uint32_t ub = 5u;
    result = (ua < ub) ? 1 : 0;
    ASSERT(result == 0, 12);
}

static void test_memory(void) {
    volatile uint32_t array[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    volatile uint32_t value;

    value = array[3];
    ASSERT(value == 3, 20);

    array[0] = 42;
    ASSERT(array[0] == 42, 21);

    volatile int8_t byte_array[4] = {-1, 0, 127, -128};
    volatile int32_t byte_val;
    byte_val = (int32_t)byte_array[0];
    ASSERT(byte_val == -1, 22);
    byte_val = (int32_t)(uint8_t)byte_array[0];
    ASSERT(byte_val == 255, 23);

    volatile int16_t half_array[4] = {-1, 0, 32767, -32768};
    volatile int32_t half_val;
    half_val = (int32_t)half_array[0];
    ASSERT(half_val == -1, 24);
    half_val = (int32_t)(uint16_t)half_array[0];
    ASSERT(half_val == 65535, 25);
}

static void test_explicit_instructions(void) {
    volatile uint8_t byte_buf[8] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff};
    int32_t val;
    uint32_t shift_result;

    __asm__ volatile ("lb %0, 0(%1)" : "=r"(val) : "r"(&byte_buf[4]));
    ASSERT(val == (int32_t)(int8_t)0x9a, 30);

    byte_buf[0] = 0xff;
    byte_buf[1] = 0xff;
    __asm__ volatile ("lh %0, 0(%1)" : "=r"(val) : "r"(byte_buf));
    ASSERT(val == -1, 31);

    __asm__ volatile ("sb %0, 2(%1)" :: "r"((uint32_t)0xAB), "r"(byte_buf));
    ASSERT(byte_buf[2] == 0xAB, 32);

    __asm__ volatile ("sh %0, 4(%1)" :: "r"((uint32_t)0x1234), "r"(byte_buf));
    ASSERT(byte_buf[4] == 0x34 && byte_buf[5] == 0x12, 33);

    __asm__ volatile ("srl %0, %1, %2" : "=r"(shift_result) : "r"((uint32_t)0x80000000), "r"((uint32_t)4));
    ASSERT(shift_result == 0x08000000, 34);

    __asm__ volatile ("sll %0, %1, %2" : "=r"(shift_result) : "r"((uint32_t)1), "r"((uint32_t)5));
    ASSERT(shift_result == 32, 35);

    __asm__ volatile ("sra %0, %1, %2" : "=r"(shift_result) : "r"((uint32_t)(int32_t)-64), "r"((uint32_t)3));
    ASSERT(shift_result == (uint32_t)(int32_t)-8, 36);
}

static void test_branches(void) {
    volatile int32_t a = 10;
    volatile int32_t b = 5;
    volatile int32_t count = 0;

    if (a == a) { count++; }
    ASSERT(count == 1, 40);

    if (a != b) { count++; }
    ASSERT(count == 2, 41);

    if (b < a) { count++; }
    ASSERT(count == 3, 42);

    if (a >= b) { count++; }
    ASSERT(count == 4, 43);

    volatile uint32_t ua = 5;
    volatile uint32_t ub = 10;
    if (ua < ub) { count++; }
    ASSERT(count == 5, 44);

    if (ub >= ua) { count++; }
    ASSERT(count == 6, 45);
}

static void test_loops(void) {
    volatile int32_t sum = 0;
    volatile int32_t i;

    for (i = 0; i < 10; i++) {
        sum += i;
    }
    ASSERT(sum == 45, 50);

    sum = 0;
    i = 0;
    while (i < 10) {
        sum += i;
        i++;
    }
    ASSERT(sum == 45, 51);
}

static int32_t add_function(int32_t a, int32_t b) {
    return a + b;
}

static int32_t __attribute__((noinline)) recursive_sum(int32_t n) {
    if (n <= 0) {
        return 0;
    }
    volatile int32_t rest = recursive_sum(n - 1);
    return n + rest;
}

static void test_functions(void) {
    volatile int32_t result;

    result = add_function(7, 8);
    ASSERT(result == 15, 60);

    result = recursive_sum(10);
    ASSERT(result == 55, 61);
}

static void test_immediates(void) {
    volatile int32_t a = 100;
    volatile int32_t result;

    result = a + 50;
    ASSERT(result == 150, 70);

    result = a & 0x0F;
    ASSERT(result == 4, 71);

    result = a | 0xF0;
    ASSERT(result == 0xF4, 72);

    result = a ^ 0xFF;
    ASSERT(result == 0x9B, 73);

    result = (a < 200) ? 1 : 0;
    ASSERT(result == 1, 74);
    result = (a < 50) ? 1 : 0;
    ASSERT(result == 0, 75);

    volatile uint32_t ua = 100;
    result = (ua < 200u) ? 1 : 0;
    ASSERT(result == 1, 76);
}

static void test_upper_immediates(void) {
    volatile uint32_t value;

    value = 0x12345000u;
    ASSERT((value >> 12) == 0x12345u, 80);

    value = 0xABCD0000u;
    ASSERT((value >> 16) == 0xABCDu, 81);

    {
        uint32_t pc1, pc2;
        __asm__ volatile (
            "auipc %0, 0\n"
            "auipc %1, 0\n"
            : "=r"(pc1), "=r"(pc2)
        );
        ASSERT(pc2 - pc1 == 4, 82);
    }
}

static volatile int32_t jump_callback_flag = 0;

static void __attribute__((noinline)) jump_target(void) {
    jump_callback_flag = 1;
}

static void test_jumps(void) {
    jump_callback_flag = 0;
    jump_target();
    ASSERT(jump_callback_flag == 1, 90);

    jump_callback_flag = 0;
    void (*fn)(void) = jump_target;
    fn();
    ASSERT(jump_callback_flag == 1, 91);
}

/* ---------- VGA tests ---------- */

#define VGA_RED_BASE    0x10000000u
#define VGA_GREEN_BASE  0x10010000u
#define VGA_BLUE_BASE   0x10020000u
#define VGA_SWAP_ADDR   0x10030000u

#define VGA_HEIGHT      120u
#define VGA_WIDTH_BYTES 80u
#define VGA_ROW_ADDR_STRIDE 0x100u
#define CPU_HZ 5000000u
/* delay_cycles() loop costs multiple instructions per iteration.
 * Calibrate iterations so wall-clock delay is close to 0.5 s at 5 MHz. */
#define DELAY_LOOP_CPI_EST 4u
#define HALF_SEC_ITERS (CPU_HZ / (2u * DELAY_LOOP_CPI_EST))

static inline uint32_t color_addr(uint32_t color_base, uint32_t y, uint32_t x_byte) {
    /* Coordinate addressing: Y selects row page, X selects byte-in-row.
     * This is intentionally not contiguous y*80 linear addressing. */
    return color_base + (y * VGA_ROW_ADDR_STRIDE) + x_byte;
}

static inline uint8_t pack_two_pixels(uint8_t even_x, uint8_t odd_x) {
    return (uint8_t)(((odd_x & 0x0Fu) << 4) | (even_x & 0x0Fu));
}

static inline void swap_frame(void) {
    *(volatile uint32_t *)VGA_SWAP_ADDR = 1u;
}

static void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

static void verify_coordinate_addressing(void) {
    uint32_t a00 = color_addr(VGA_RED_BASE, 0u, 0u);
    uint32_t a01 = color_addr(VGA_RED_BASE, 0u, 1u);
    uint32_t a10 = color_addr(VGA_RED_BASE, 1u, 0u);

    ASSERT(a00 == VGA_RED_BASE, 103);
    ASSERT((a01 - a00) == 1u, 104);
    ASSERT((a10 - a00) == VGA_ROW_ADDR_STRIDE, 105);
    ASSERT((color_addr(VGA_RED_BASE, 119u, 79u) - VGA_RED_BASE) == 0x774Fu, 106);
}

static void vga_store_size_smoke_test(void) {
    volatile uint8_t *r8 = (volatile uint8_t *)color_addr(VGA_RED_BASE, 0u, 0u);
    volatile uint16_t *r16 = (volatile uint16_t *)color_addr(VGA_RED_BASE, 0u, 2u);
    volatile uint32_t *r32 = (volatile uint32_t *)color_addr(VGA_RED_BASE, 0u, 4u);

    *r8 = 0xA5u;
    *r16 = 0x5AA5u;
    *r32 = 0x12345678u;
}

static uint8_t frame_a_red(uint32_t xb) {
    uint8_t x0 = (uint8_t)(xb << 1);
    uint8_t x1 = (uint8_t)(x0 + 1u);
    return pack_two_pixels((uint8_t)(x0 >> 4), (uint8_t)(x1 >> 4));
}

static uint8_t frame_a_green(uint32_t y) {
    uint8_t g = (uint8_t)(y >> 3);
    return pack_two_pixels(g, g);
}

static uint8_t frame_a_blue(void) {
    return pack_two_pixels(0x2u, 0x2u);
}

static uint8_t frame_b_red(uint32_t y, uint32_t xb) {
    uint8_t checker = (uint8_t)(((xb ^ y) & 1u) ? 0xEu : 0x1u);
    return pack_two_pixels(checker, checker);
}

static uint8_t frame_b_green(void) {
    return pack_two_pixels(0x0u, 0x0u);
}

static uint8_t frame_b_blue(uint32_t y, uint32_t xb) {
    uint8_t checker = (uint8_t)(((xb ^ y) & 1u) ? 0xEu : 0x1u);
    uint8_t inv = (uint8_t)(0xFu - checker);
    return pack_two_pixels(inv, inv);
}

static void draw_frame_a(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t xb = 0; xb < VGA_WIDTH_BYTES; xb++) {
            *(volatile uint8_t *)color_addr(VGA_RED_BASE, y, xb) = frame_a_red(xb);
            *(volatile uint8_t *)color_addr(VGA_GREEN_BASE, y, xb) = frame_a_green(y);
            *(volatile uint8_t *)color_addr(VGA_BLUE_BASE, y, xb) = frame_a_blue();
        }
    }
}

static void draw_frame_b(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t xb = 0; xb < VGA_WIDTH_BYTES; xb++) {
            *(volatile uint8_t *)color_addr(VGA_RED_BASE, y, xb) = frame_b_red(y, xb);
            *(volatile uint8_t *)color_addr(VGA_GREEN_BASE, y, xb) = frame_b_green();
            *(volatile uint8_t *)color_addr(VGA_BLUE_BASE, y, xb) = frame_b_blue(y, xb);
        }
    }
}

static void draw_fail_screen_red(void) {
    uint8_t red = pack_two_pixels(0xFu, 0xFu);
    uint8_t zero = pack_two_pixels(0x0u, 0x0u);
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t xb = 0; xb < VGA_WIDTH_BYTES; xb++) {
            *(volatile uint8_t *)color_addr(VGA_RED_BASE, y, xb) = red;
            *(volatile uint8_t *)color_addr(VGA_GREEN_BASE, y, xb) = zero;
            *(volatile uint8_t *)color_addr(VGA_BLUE_BASE, y, xb) = zero;
        }
    }
}

int main(void) {
    test_result = 0;
    test_passed = 0;
    test_failed = 0;
    fail_code = 0;
    vga_stage = 0;

    /* ISA regression */
    test_arithmetic();
    if (test_result) { goto fail_screen; }
    test_memory();
    if (test_result) { goto fail_screen; }
    test_explicit_instructions();
    if (test_result) { goto fail_screen; }
    test_branches();
    if (test_result) { goto fail_screen; }
    test_loops();
    if (test_result) { goto fail_screen; }
    test_functions();
    if (test_result) { goto fail_screen; }
    test_immediates();
    if (test_result) { goto fail_screen; }
    test_upper_immediates();
    if (test_result) { goto fail_screen; }
    test_jumps();
    if (test_result) { goto fail_screen; }

    /* VGA regression */
    verify_coordinate_addressing();
    if (test_result) { goto fail_screen; }
    vga_stage = 1; /* address mapping checks passed */
    vga_store_size_smoke_test();
    if (test_result) { goto fail_screen; }
    vga_stage = 2; /* smoke writes completed */

    /* Success mode: continuously swap test patterns every 0.5 s at 5 MHz. */
    for (;;) {
        draw_frame_a();
        vga_stage = 3; /* frame A written */
        swap_frame();
        delay_cycles(HALF_SEC_ITERS);

        draw_frame_b();
        vga_stage = 4; /* frame B written */
        swap_frame();
        delay_cycles(HALF_SEC_ITERS);
    }

fail_screen:
    /* Failure mode: show solid red and hold. */
    vga_stage = 0xFFu;
    draw_fail_screen_red();
    swap_frame();
    for (;;) {
        delay_cycles(HALF_SEC_ITERS);
    }
}
