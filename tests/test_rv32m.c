/*
 * RV32M multiply/divide extension test — bare-metal CPU verification.
 *
 * Default march is rv32im. For rv32i-only sim: make RV32I_ONLY=1 PROGRAM=test_rv32m
 *
 * Instructions (forced via asm): MUL MULH MULHSU MULHU DIV DIVU REM REMU
 *
 * main() runs rv32m_waveform_burst() first: one asm block emits all eight
 * mnemonics consecutively (handy waveform landmark during simulation).
 *
 * test_mul_family checks low-half carry into the high limb, signed and unsigned
 * high-half multiplies, and (when __int128 is available) wide mulhsu goldens.
 */

#include <stdint.h>

volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;

/* XOR of burst outputs so the landmark isn't optimized away */
volatile uint32_t rv32m_waveform_burst_sink = 0;

/*
 * Landmark for simulation: emit MUL*, DIV*, REM* back-to-back. Operands are
 * chosen only so register results are visually “busy” in waves — not paired
 * with the ASSERT suite.
 */
static void rv32m_waveform_burst(void) {
    /* Large mixed patterns: low/high mul limbs and mulhsu all non-trivial */
    uint32_t mul_a = 0xffff0009u;
    uint32_t mul_b = 0x00020003u;
    /* Signed DIV/REM: quotient and remainder both non-zero */
    uint32_t div_n = (uint32_t)(int32_t)(-1000003);
    uint32_t div_d = (uint32_t)(int32_t)(401);
    /* Unsigned DIVU/REMU: big dividend, remainder ≠ 0 */
    uint32_t udiv_n = 0xcafeba9eu;
    uint32_t udiv_d = 0x1337u;

    uint32_t r_mul, r_mulh, r_mulhu, r_mulhsu;
    uint32_t r_div, r_divu, r_rem, r_remu;

    __asm__ volatile(
        "# spellbook RV32M burst (waveform landmark)\n\t"
        "mul %[Rm], %[Ma], %[Mb]\n\t"
        "mulh %[Rh], %[Ma], %[Mb]\n\t"
        "mulhu %[Rhu], %[Ma], %[Mb]\n\t"
        "mulhsu %[Rhsu], %[Ma], %[Mb]\n\t"
        "div %[Rd], %[Dn], %[Dd]\n\t"
        "divu %[Rdu], %[Un], %[Ud]\n\t"
        "rem %[Rr], %[Dn], %[Dd]\n\t"
        "remu %[Rru], %[Un], %[Ud]"
        : [Rm]"=&r"(r_mul),
          [Rh]"=&r"(r_mulh),
          [Rhu]"=&r"(r_mulhu),
          [Rhsu]"=&r"(r_mulhsu),
          [Rd]"=&r"(r_div),
          [Rdu]"=&r"(r_divu),
          [Rr]"=&r"(r_rem),
          [Rru]"=&r"(r_remu)
        : [Ma]"r"(mul_a),
          [Mb]"r"(mul_b),
          [Dn]"r"(div_n),
          [Dd]"r"(div_d),
          [Un]"r"(udiv_n),
          [Ud]"r"(udiv_d)
        : );

    rv32m_waveform_burst_sink =
        r_mul ^ r_mulh ^ r_mulhu ^ r_mulhsu ^
        r_div ^ r_divu ^ r_rem ^ r_remu;
}

#define ASSERT(condition, _label) \
    do { \
        if (condition) { \
            test_passed++; \
        } else { \
            test_failed++; \
            test_result = 1; \
        } \
    } while (0)

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

#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ >= 16)
/* Golden for mulhsu: sign-extend rs1, zero-extend rs2, take bits [63:32] of product. */
static uint32_t expect_mulhsu32(int32_t s1, uint32_t u2) {
    __int128 ss = (__int128)s1;
    __uint128 uz = (__uint128)(uint64_t)u2;
    __int128 prod = ss * (__int128)uz;
    return (uint32_t)(((__uint128)prod) >> 32);
}
#endif

static void test_mul_family(void) {
    /* MUL: low 32 bits (Z/2^32Z); include case where high limb of u64 product != 0 */
    volatile uint32_t a = 70003u;
    volatile uint32_t b = 489u;
    uint32_t lo_expect = (uint32_t)(((uint64_t)a * (uint64_t)b) & 0xFFFFFFFFull);
    ASSERT(asm_mul(a, b) == lo_expect, "MUL");

    volatile uint32_t lo_big_a = 0x80010000u;
    volatile uint32_t lo_big_b = 3u;
    lo_expect =
        (uint32_t)(((uint64_t)lo_big_a * (uint64_t)lo_big_b) & 0xFFFFFFFFull);
    ASSERT(asm_mul(lo_big_a, lo_big_b) == lo_expect, "MUL high limb carry");

    /* Same bit patterns as signed int32: low limb still matches unsigned residue */
    volatile int32_t lo_sa = (int32_t)0x80010000u;
    volatile int32_t lo_sb = 3;
    ASSERT(asm_mul((uint32_t)lo_sa, (uint32_t)lo_sb) == lo_expect, "MUL signed patterns");

    /* MULH: signed × signed with non-trivial high limb (large positives) */
    volatile int32_t sh_a = (int32_t)0x30004000;
    volatile int32_t sh_b = (int32_t)0x05006000;
    uint32_t mh_expect =
        (uint32_t)((((int64_t)sh_a * (int64_t)sh_b) >> 32) & 0xFFFFFFFFll);
    ASSERT(asm_mulh((uint32_t)sh_a, (uint32_t)sh_b) == mh_expect, "MULH pos×pos");

    /* MULH: negative × positive (product fits int64; high half not just 0/−1) */
    volatile int32_t sa = -12345;
    volatile int32_t sb = 901;
    mh_expect = (uint32_t)(((int64_t)sa * (int64_t)sb) >> 32);
    ASSERT(asm_mulh((uint32_t)sa, (uint32_t)sb) == mh_expect, "MULH neg×pos");

    /* MULHU: unsigned × unsigned, high limb clearly non-zero */
    volatile uint32_t ua = 0xffff0009u;
    volatile uint32_t ub = 0x00020003u;
    uint32_t hu_expect =
        (uint32_t)(((uint64_t)ua * (uint64_t)ub) >> 32);
    ASSERT(asm_mulhu(ua, ub) == hu_expect, "MULHU");

    /* MULHSU: small operands (golden fits int64 arithmetic) */
    volatile int32_t s_neg = -3;
    volatile uint32_t u_small = 5u;
    uint32_t hsu_expect =
        (uint32_t)((((int64_t)s_neg * (uint64_t)u_small) >> 32) & 0xFFFFFFFFll);
    ASSERT(asm_mulhsu((uint32_t)s_neg, u_small) == hsu_expect, "MULHSU small");

#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ >= 16)
    /* Larger mulhsu: product needs full sext × zext; still fits __int128 */
    volatile int32_t hs1 = (int32_t)0x80000001; /* −2147483647 */
    volatile uint32_t hs2 = 0xffffffffu;
    ASSERT(
        asm_mulhsu((uint32_t)hs1, hs2) == expect_mulhsu32(hs1, hs2),
        "MULHSU large u2"
    );

    volatile int32_t hs3 = -2;
    volatile uint32_t hs4 = 1u << 31;
    ASSERT(
        asm_mulhsu((uint32_t)hs3, hs4) == expect_mulhsu32(hs3, hs4),
        "MULHSU neg×2^31"
    );
#endif
}

static void test_div_family(void) {
    volatile int32_t n = -2147483628;
    volatile int32_t d_pos = -19;
    int32_t dq = n / d_pos;
    ASSERT((int32_t)asm_div((uint32_t)n, (uint32_t)d_pos) == dq, "DIV quot");

    volatile uint32_t un = 0xffffffeeu;
    volatile uint32_t ud = 7u;
    ASSERT(asm_divu(un, ud) == (un / ud), "DIVU");

    volatile int32_t rn = -100;
    volatile int32_t rd = 7;
    ASSERT((int32_t)asm_rem((uint32_t)rn, (uint32_t)rd) == (rn % rd), "REM");

    volatile uint32_t urn = 0xffffffebu;
    volatile uint32_t urd = 51u;
    ASSERT(asm_remu(urn, urd) == (urn % urd), "REMU");

    volatile int32_t vq = -9;
    volatile int32_t vr = -2;
    ASSERT((int32_t)asm_rem((uint32_t)vq, (uint32_t)vr) == (vq % vr), "REM signs");
}

static void test_div_trunc_towards_zero(void) {
    volatile int32_t n = -7;
    volatile int32_t d = 3;
    ASSERT((int32_t)asm_div((uint32_t)n, (uint32_t)d) == -2, "DIV truncate tz");
}

int main(void) {
    test_passed = 0;
    test_failed = 0;
    test_result = 0;

    rv32m_waveform_burst();

    test_mul_family();
    test_div_family();
    test_div_trunc_towards_zero();

    return test_result;
}
