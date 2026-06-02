/*
 * Combined RV32I + RV32M + VGA regression (default: rv32im).
 *
 * ISA coverage (I + M), then VGA smoke + equal-band gradient with blue triangle
 * wave 0→15→0 (~1 s rise + ~1 s fall, 16 RGB444 steps). Build rv32i-only snapshot:
 *   make RV32I_ONLY=1 PROGRAM=test_isa_vga_rv32i
 */

#include <stdint.h>
#include "vga_driver.h"
#include "delay.h"

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

/* ---------- RV32M tests (requires -march rv32im) ---------- */

#if defined(__riscv_mul) && defined(__riscv_div)

static uint32_t asm_mul(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("mul %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_mulh(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("mulh %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_mulhu(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("mulhu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_mulhsu(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("mulhsu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_div(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("div %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_divu(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("divu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_rem(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("rem %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asm_remu(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm__ volatile ("remu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static void rv32m_waveform_burst(void) {
    volatile uint32_t a = 0x0000FFFFu;
    volatile uint32_t b = 0x00010001u;
    volatile uint32_t c = 0x0000FFFEu;
    volatile uint32_t d = 0x00010002u;
    volatile uint32_t acc = 0u;

    acc += asm_mul(a, b);
    acc += asm_mulh((uint32_t)(int32_t)-1, 3u);
    acc += asm_mulhu(0xFFFF0000u, 0x0000FFFFu);
    acc += asm_mulhsu((uint32_t)(int32_t)-2, 0x80000000u);
    acc += asm_div((uint32_t)(int32_t)-2147483648, (uint32_t)(int32_t)-1);
    acc += asm_divu(0xFFFFFFFEu, 3u);
    acc += asm_rem((uint32_t)(int32_t)-7, 3u);
    acc += asm_remu(0xFFFFFFFDu, 5u);

    if (acc == 0u) {
        acc = asm_mul(c, d);
    }
    (void)acc;
}

#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ >= 16)
static uint32_t expect_mulhsu32(int32_t s1, uint32_t u2) {
    __int128 ss = (__int128)s1;
    __uint128 uz = (__uint128)(uint64_t)u2;
    __int128 prod = ss * (__int128)uz;
    return (uint32_t)(((__uint128)prod) >> 32);
}
#endif

static void test_mul_family(void) {
    volatile uint32_t a = 70003u;
    volatile uint32_t b = 489u;
    uint32_t lo_expect = (uint32_t)(((uint64_t)a * (uint64_t)b) & 0xFFFFFFFFull);
    ASSERT(asm_mul(a, b) == lo_expect, 200);

    volatile uint32_t lo_big_a = 0x80010000u;
    volatile uint32_t lo_big_b = 3u;
    lo_expect =
        (uint32_t)(((uint64_t)lo_big_a * (uint64_t)lo_big_b) & 0xFFFFFFFFull);
    ASSERT(asm_mul(lo_big_a, lo_big_b) == lo_expect, 201);

    volatile int32_t lo_sa = (int32_t)0x80010000u;
    volatile int32_t lo_sb = 3;
    ASSERT(asm_mul((uint32_t)lo_sa, (uint32_t)lo_sb) == lo_expect, 202);

    volatile int32_t sh_a = (int32_t)0x30004000;
    volatile int32_t sh_b = (int32_t)0x05006000;
    uint32_t mh_expect =
        (uint32_t)((((int64_t)sh_a * (int64_t)sh_b) >> 32) & 0xFFFFFFFFll);
    ASSERT(asm_mulh((uint32_t)sh_a, (uint32_t)sh_b) == mh_expect, 203);

    volatile int32_t sa = -12345;
    volatile int32_t sb = 901;
    mh_expect = (uint32_t)(((int64_t)sa * (int64_t)sb) >> 32);
    ASSERT(asm_mulh((uint32_t)sa, (uint32_t)sb) == mh_expect, 204);

    volatile uint32_t ua = 0xffff0009u;
    volatile uint32_t ub = 0x00020003u;
    uint32_t hu_expect =
        (uint32_t)(((uint64_t)ua * (uint64_t)ub) >> 32);
    ASSERT(asm_mulhu(ua, ub) == hu_expect, 205);

    volatile int32_t s_neg = -3;
    volatile uint32_t u_small = 5u;
    uint32_t hsu_expect =
        (uint32_t)((((int64_t)s_neg * (uint64_t)u_small) >> 32) & 0xFFFFFFFFll);
    ASSERT(asm_mulhsu((uint32_t)s_neg, u_small) == hsu_expect, 206);

#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ >= 16)
    volatile int32_t hs1 = (int32_t)0x80000001;
    volatile uint32_t hs2 = 0xffffffffu;
    ASSERT(
        asm_mulhsu((uint32_t)hs1, hs2) == expect_mulhsu32(hs1, hs2),
        207
    );

    volatile int32_t hs3 = -2;
    volatile uint32_t hs4 = 1u << 31;
    ASSERT(
        asm_mulhsu((uint32_t)hs3, hs4) == expect_mulhsu32(hs3, hs4),
        208
    );
#endif
}

static void test_div_family(void) {
    volatile int32_t n = -2147483628;
    volatile int32_t d_pos = -19;
    int32_t dq = n / d_pos;
    ASSERT((int32_t)asm_div((uint32_t)n, (uint32_t)d_pos) == dq, 210);

    volatile uint32_t un = 0xffffffeeu;
    volatile uint32_t ud = 7u;
    ASSERT(asm_divu(un, ud) == (un / ud), 211);

    volatile int32_t rn = -100;
    volatile int32_t rd = 7;
    ASSERT((int32_t)asm_rem((uint32_t)rn, (uint32_t)rd) == (rn % rd), 212);

    volatile uint32_t urn = 0xffffffebu;
    volatile uint32_t urd = 51u;
    ASSERT(asm_remu(urn, urd) == (urn % urd), 213);

    volatile int32_t vq = -9;
    volatile int32_t vr = -2;
    ASSERT((int32_t)asm_rem((uint32_t)vq, (uint32_t)vr) == (vq % vr), 214);
}

static void test_div_trunc_towards_zero(void) {
    volatile int32_t n = -7;
    volatile int32_t d = 3;
    ASSERT((int32_t)asm_div((uint32_t)n, (uint32_t)d) == -2, 215);
}

static void test_rv32m(void) {
    rv32m_waveform_burst();
    test_mul_family();
    test_div_family();
    test_div_trunc_towards_zero();
}

#endif /* __riscv_mul && __riscv_div */

/* ---------- VGA tests ---------- */

static void vga_store_size_smoke_test(void) {
    /* Store-size probe: one byte, halfword, word at (0,0) — each a single insn */
    MMIO_STORE8(__vga_fb_base, 0, 0xA5u);
    MMIO_STORE16(__vga_fb_base, 2, 0x5AA5u);
    MMIO_STORE32(__vga_fb_base, 4, 0x12345678u);
}

static void verify_coordinate_addressing(void) {
    uint32_t a00 = vga_fb_addr_fast(0u, 0u);
    uint32_t a01 = vga_fb_addr_fast(1u, 0u);
    uint32_t a10 = vga_fb_addr_fast(0u, 1u);

    ASSERT(a00 == VGA_FB_BASE, 103);
    ASSERT((a01 - a00) == 1u, 104);
    ASSERT((a10 - a00) == VGA_ROW_ADDR_STRIDE, 105);
    ASSERT((vga_fb_addr_fast(159u, 119u) - VGA_FB_BASE) == 0x779Fu, 106);
}

/*
 * White (RGB 0xFFF) at (0,0); pure blue (RGB 0x00F) at bottom-right (159,119).
 * Sixteen equal-ish bands per axis (R: 10 px each; G: 8/7 px alternating).
 */
static void __attribute__((noinline)) draw_gradient_frame(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        uint8_t g = vga_gradient_g_equal_band_fast(y);
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            uint8_t r = vga_gradient_r_equal_band_fast(x);
            vga_write_index_fast(x, y, vga_palette_index_from_rg_fast(r, g));
        }
    }
}

/*
 * Seed both frame buffers once; palette is shared so blue animation needs no
 * per-frame FB redraw (avoids alternating blank/index-0 buffer flashes).
 */
static void init_vga_gradient_demo(uint8_t blue) {
    vga_init_palette_rg_b_fast(blue);
    draw_gradient_frame();
    swap_frame();
    draw_gradient_frame();
    swap_frame();
}

static void draw_fail_screen_red(void) {
    vga_fill_screen_fail_red_fast();
}

int main(void) {
    uint8_t blue = 8u;
    int8_t blue_dir = 1;

    test_result = 0;
    test_passed = 0;
    test_failed = 0;
    fail_code = 0;
    vga_stage = 0;

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
#if defined(__riscv_mul) && defined(__riscv_div)
    test_rv32m();
    if (test_result) { goto fail_screen; }
#endif

    verify_coordinate_addressing();
    if (test_result) { goto fail_screen; }
    vga_stage = 1;
    vga_store_size_smoke_test();
    if (test_result) { goto fail_screen; }
    vga_stage = 2;
    init_vga_gradient_demo(blue);

    for (;;) {
        vga_init_palette_rg_b_fast(blue);
        swap_frame();
        vga_stage = 3;
        delay_cpu_instructions(DELAY_BLUE_STEP_ITERS);

        if (blue_dir > 0) {
            if (blue < 15u) {
                blue++;
            } else {
                blue_dir = -1;
            }
        } else {
            if (blue > 0u) {
                blue--;
            } else {
                blue_dir = 1;
            }
        }
    }

fail_screen:
    vga_stage = 0xFFu;
    draw_fail_screen_red();
    swap_frame();
    for (;;) {
        delay_cpu_instructions(DELAY_ONE_SEC_ITERS);
    }
}
