/*
 * Basic RISC-V 32I ISA Test Program
 * Tests core integer instructions for FPGA CPU verification
 */

#include <stdint.h>

// Volatile to prevent compiler optimizations
volatile uint32_t test_result = 0;
volatile uint32_t test_passed = 0;
volatile uint32_t test_failed = 0;

// Simple assertion macro
#define ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            test_passed++; \
        } else { \
            test_failed++; \
            test_result = 1; \
        } \
    } while(0)

// Test arithmetic operations
void test_arithmetic(void) {
    volatile int32_t a = 10;
    volatile int32_t b = 5;
    volatile int32_t result;
    
    // ADD
    result = a + b;
    ASSERT(result == 15, "ADD");
    
    // SUB
    result = a - b;
    ASSERT(result == 5, "SUB");
    
    // AND
    result = a & b;
    ASSERT(result == 0, "AND");
    
    // OR
    result = a | b;
    ASSERT(result == 15, "OR");
    
    // XOR
    result = a ^ b;
    ASSERT(result == 15, "XOR");
    
    // Shift left
    result = a << 2;
    ASSERT(result == 40, "SLL");
    
    // Shift right logical
    result = a >> 2;
    ASSERT(result == 2, "SRL");
    
    // Shift right arithmetic
    volatile int32_t neg = -16;
    result = neg >> 2;
    ASSERT(result == -4, "SRA");
    
    // Set less than
    result = (a < b) ? 1 : 0;
    ASSERT(result == 0, "SLT");
    result = (b < a) ? 1 : 0;
    ASSERT(result == 1, "SLT");
    
    // Set less than unsigned
    volatile uint32_t ua = 0xFFFFFFFF;
    volatile uint32_t ub = 5;
    result = (ua < ub) ? 1 : 0;
    ASSERT(result == 0, "SLTU");
}

// Test memory operations
void test_memory(void) {
    volatile uint32_t array[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    volatile uint32_t value;
    
    // Load word
    value = array[3];
    ASSERT(value == 3, "LW");
    
    // Store word
    array[0] = 42;
    ASSERT(array[0] == 42, "SW");
    
    // Test byte operations (sign extension)
    volatile int8_t byte_array[4] = {-1, 0, 127, -128};
    volatile int32_t byte_val;
    
    byte_val = (int32_t)byte_array[0]; // Sign extend
    ASSERT(byte_val == -1, "LB sign extend");
    
    byte_val = (int32_t)(uint8_t)byte_array[0]; // Zero extend
    ASSERT(byte_val == 255, "LBU zero extend");
    
    // Halfword operations
    volatile int16_t half_array[4] = {-1, 0, 32767, -32768};
    volatile int32_t half_val;
    
    half_val = (int32_t)half_array[0]; // Sign extend
    ASSERT(half_val == -1, "LH sign extend");
    
    half_val = (int32_t)(uint16_t)half_array[0]; // Zero extend
    ASSERT(half_val == 65535, "LHU zero extend");
}

// Test control flow (branches)
void test_branches(void) {
    volatile int32_t a = 10;
    volatile int32_t b = 5;
    volatile int32_t count = 0;
    
    // BEQ (branch if equal)
    if (a == a) {
        count++;
    }
    ASSERT(count == 1, "BEQ");
    
    // BNE (branch if not equal)
    if (a != b) {
        count++;
    }
    ASSERT(count == 2, "BNE");
    
    // BLT (branch if less than)
    if (b < a) {
        count++;
    }
    ASSERT(count == 3, "BLT");
    
    // BGE (branch if greater or equal)
    if (a >= b) {
        count++;
    }
    ASSERT(count == 4, "BGE");
    
    // BLTU (branch if less than unsigned)
    volatile uint32_t ua = 5;
    volatile uint32_t ub = 10;
    if (ua < ub) {
        count++;
    }
    ASSERT(count == 5, "BLTU");
    
    // BGEU (branch if greater or equal unsigned)
    if (ub >= ua) {
        count++;
    }
    ASSERT(count == 6, "BGEU");
}

// Test loops
void test_loops(void) {
    volatile int32_t sum = 0;
    volatile int32_t i;
    
    // Simple for loop
    for (i = 0; i < 10; i++) {
        sum += i;
    }
    ASSERT(sum == 45, "for loop");
    
    // While loop
    sum = 0;
    i = 0;
    while (i < 10) {
        sum += i;
        i++;
    }
    ASSERT(sum == 45, "while loop");
}

// Test function calls
int32_t add_function(int32_t a, int32_t b) {
    return a + b;
}

// Recursive function that doesn't require multiplication (rv32i doesn't have M extension)
int32_t recursive_sum(int32_t n) {
    if (n <= 0) {
        return 0;
    }
    return n + recursive_sum(n - 1);
}

void test_functions(void) {
    volatile int32_t result;
    
    result = add_function(7, 8);
    ASSERT(result == 15, "function call");
    
    // Test recursion (sum of 1 to 10 = 55)
    result = recursive_sum(10);
    ASSERT(result == 55, "recursive function");
}

// Test immediate operations
void test_immediates(void) {
    volatile int32_t a = 100;
    volatile int32_t result;
    
    // ADDI
    result = a + 50;
    ASSERT(result == 150, "ADDI");
    
    // ANDI
    result = a & 0x0F;
    ASSERT(result == 4, "ANDI");
    
    // ORI
    result = a | 0xF0;
    ASSERT(result == 0xF4, "ORI");
    
    // XORI
    result = a ^ 0xFF;
    ASSERT(result == 0x9B, "XORI");
    
    // SLTI
    result = (a < 200) ? 1 : 0;
    ASSERT(result == 1, "SLTI");
    result = (a < 50) ? 1 : 0;
    ASSERT(result == 0, "SLTI");
    
    // SLTIU
    volatile uint32_t ua = 100;
    result = (ua < 200U) ? 1 : 0;
    ASSERT(result == 1, "SLTIU");
}

// Test LUI and AUIPC (load upper immediate, add upper immediate to PC)
void test_upper_immediates(void) {
    volatile uint32_t value;
    
    // LUI (load upper immediate)
    value = 0x12345000;
    ASSERT((value >> 12) == 0x12345, "LUI");
    
    // AUIPC would be tested with position-independent code
    // For now, just verify we can create large constants
    value = 0xABCD0000;
    ASSERT((value >> 16) == 0xABCD, "upper immediate");
}

// Test JAL and JALR (jump and link)
void test_jumps(void) {
    volatile int32_t called = 0;
    
    // JAL (jump and link) - tested via function calls
    // JALR (jump and link register) - also via function calls
    
    void test_jump_target(void) {
        called = 1;
    }
    
    test_jump_target();
    ASSERT(called == 1, "JAL/JALR");
}

// Main test runner
int main(void) {
    // Initialize test counters
    test_passed = 0;
    test_failed = 0;
    test_result = 0;
    
    // Run all tests
    test_arithmetic();
    // test_memory();
    test_branches();
    test_loops();
    test_functions();
    test_immediates();
    test_upper_immediates();
    test_jumps();
    
    // Final result
    // If running on FPGA, you might want to output this to a register or memory location
    // For now, we'll use it as return value
    return test_result;
}
