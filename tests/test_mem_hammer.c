/*
 * Aggressive memory stress test for RV32I systems.
 *
 * Focus:
 *  - Detect wrong-address writes and read/write corruption
 *  - Exercise word/halfword/byte paths
 *  - Hammer memory with repeated randomized traffic
 */

#include <stdint.h>

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;

volatile uint32_t fail_phase = 0;
volatile uint32_t fail_index = 0;
volatile uint32_t fail_expected = 0;
volatile uint32_t fail_actual = 0;

#define ASSERT_EQ(actual, expected, phase_id, index_id) \
    do { \
        uint32_t _a = (uint32_t)(actual); \
        uint32_t _e = (uint32_t)(expected); \
        if (_a == _e) { \
            test_passed++; \
        } else { \
            test_failed++; \
            test_result = 1; \
            fail_phase = (phase_id); \
            fail_index = (index_id); \
            fail_expected = _e; \
            fail_actual = _a; \
            return; \
        } \
    } while (0)

#define MEM_WORDS 512u
#define GUARD_WORDS 8u
#define MEM_MASK (MEM_WORDS - 1u)

#define GUARD_LO_PATTERN 0xCAFEBABEu
#define GUARD_HI_PATTERN 0xDEADC0DEu

typedef struct {
    volatile uint32_t guard_lo[GUARD_WORDS];
    volatile uint32_t mem[MEM_WORDS];
    volatile uint32_t guard_hi[GUARD_WORDS];
} mem_region_t;

static mem_region_t region;
static uint32_t shadow[MEM_WORDS];

static uint32_t xorshift32(uint32_t state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static uint32_t rol32(uint32_t v, uint32_t sh) {
    sh &= 31u;
    if (sh == 0u) {
        return v;
    }
    return (v << sh) | (v >> (32u - sh));
}

static void init_guards(void) {
    uint32_t i;
    for (i = 0; i < GUARD_WORDS; i++) {
        region.guard_lo[i] = GUARD_LO_PATTERN ^ (i * 0x11111111u);
        region.guard_hi[i] = GUARD_HI_PATTERN ^ (i * 0x01010101u);
    }
}

static void check_guards(uint32_t phase_id) {
    uint32_t i;
    for (i = 0; i < GUARD_WORDS; i++) {
        ASSERT_EQ(region.guard_lo[i], GUARD_LO_PATTERN ^ (i * 0x11111111u), phase_id, i);
        ASSERT_EQ(region.guard_hi[i], GUARD_HI_PATTERN ^ (i * 0x01010101u), phase_id, i);
    }
}

static void fill_and_check_patterns(void) {
    uint32_t i;
    uint32_t pass;
    uint32_t p;

    for (i = 0; i < MEM_WORDS; i++) {
        region.mem[i] = 0u;
        shadow[i] = 0u;
    }

    check_guards(1u);

    for (pass = 0; pass < 16u; pass++) {
        for (i = 0; i < MEM_WORDS; i++) {
            p = (0xA5A50000u | pass) ^ (i << (pass & 7u)) ^ rol32(i, pass & 31u);
            region.mem[i] = p;
            shadow[i] = p;
        }
        for (i = 0; i < MEM_WORDS; i++) {
            ASSERT_EQ(region.mem[i], shadow[i], 2u, i);
        }
        check_guards(3u);
    }
}

static void march_like_test(void) {
    uint32_t i;

    for (i = 0; i < MEM_WORDS; i++) {
        region.mem[i] = 0u;
    }

    for (i = 0; i < MEM_WORDS; i++) {
        ASSERT_EQ(region.mem[i], 0u, 10u, i);
        region.mem[i] = 0xFFFFFFFFu;
    }

    for (i = 0; i < MEM_WORDS; i++) {
        ASSERT_EQ(region.mem[i], 0xFFFFFFFFu, 11u, i);
        region.mem[i] = 0u;
    }

    for (i = MEM_WORDS; i > 0u; i--) {
        uint32_t idx = i - 1u;
        ASSERT_EQ(region.mem[idx], 0u, 12u, idx);
        region.mem[idx] = 0xFFFFFFFFu;
    }

    for (i = MEM_WORDS; i > 0u; i--) {
        uint32_t idx = i - 1u;
        ASSERT_EQ(region.mem[idx], 0xFFFFFFFFu, 13u, idx);
        region.mem[idx] = 0u;
    }

    for (i = 0; i < MEM_WORDS; i++) {
        ASSERT_EQ(region.mem[i], 0u, 14u, i);
    }

    check_guards(15u);
}

static void byte_halfword_alias_test(void) {
    volatile uint8_t *b = (volatile uint8_t *)&region.mem[0];
    volatile uint16_t *h = (volatile uint16_t *)&region.mem[0];
    uint32_t i;
    uint32_t expected;

    for (i = 0; i < MEM_WORDS; i++) {
        region.mem[i] = 0u;
    }

    for (i = 0; i < MEM_WORDS * 4u; i++) {
        b[i] = (uint8_t)((i * 37u) ^ 0x5Au);
    }

    for (i = 0; i < MEM_WORDS; i++) {
        uint32_t base = i * 4u;
        expected =
            (uint32_t)b[base] |
            ((uint32_t)b[base + 1u] << 8) |
            ((uint32_t)b[base + 2u] << 16) |
            ((uint32_t)b[base + 3u] << 24);
        ASSERT_EQ(region.mem[i], expected, 20u, i);
    }

    for (i = 0; i < MEM_WORDS * 2u; i++) {
        h[i] = (uint16_t)((i * 149u) ^ 0xA55Au);
    }

    for (i = 0; i < MEM_WORDS; i++) {
        uint32_t lo = (uint32_t)h[i * 2u];
        uint32_t hi = (uint32_t)h[i * 2u + 1u];
        expected = lo | (hi << 16);
        ASSERT_EQ(region.mem[i], expected, 21u, i);
    }

    check_guards(22u);
}

static void randomized_hammer_test(void) {
    uint32_t i;
    uint32_t iter;
    uint32_t state = 0x1BADF00Du;

    for (i = 0; i < MEM_WORDS; i++) {
        uint32_t v = 0x13579BDFu ^ rol32(i, i & 31u);
        region.mem[i] = v;
        shadow[i] = v;
    }

    for (iter = 0; iter < 300000u; iter++) {
        uint32_t idx;
        uint32_t value;
        uint32_t v_idx;

        state = xorshift32(state);
        idx = state & MEM_MASK;
        value = state ^ rol32(idx, (state >> 27u) & 31u) ^ 0x9E3779B9u;

        region.mem[idx] = value;
        shadow[idx] = value;

        ASSERT_EQ(region.mem[idx], shadow[idx], 30u, idx);

        state = xorshift32(state);
        v_idx = state & MEM_MASK;
        ASSERT_EQ(region.mem[v_idx], shadow[v_idx], 31u, v_idx);

        if ((iter & 0x7FFu) == 0u) {
            for (i = 0; i < MEM_WORDS; i++) {
                ASSERT_EQ(region.mem[i], shadow[i], 32u, i);
            }
            check_guards(33u);
        }
    }

    for (i = 0; i < MEM_WORDS; i++) {
        ASSERT_EQ(region.mem[i], shadow[i], 34u, i);
    }

    check_guards(35u);
}

int main(void) {
    test_result = 0;
    test_passed = 0;
    test_failed = 0;
    fail_phase = 0;
    fail_index = 0;
    fail_expected = 0;
    fail_actual = 0;

    init_guards();
    check_guards(0u);

    fill_and_check_patterns();
    march_like_test();
    byte_halfword_alias_test();
    randomized_hammer_test();

    return (int)test_result;
}
