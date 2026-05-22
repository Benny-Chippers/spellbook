#include <stdint.h>

volatile uint8_t* data_mem = (uint8_t*)0x10000000;
volatile uint32_t* test_passed;
volatile uint32_t* test_failed;

int main(void) {
    test_passed = (uint32_t*)0x60000504;
    test_failed = (uint32_t*)0x60000008;

    *test_passed = 0;
    *test_failed = 0;

    *test_passed = 0x12345678;
    *test_failed = *test_passed + 1;

    // for (int i = 0; i < 16; i++) {
    //     data_mem[i] = i;
    // }
    // for (int i = 0; i < 16; i++) {
    //     if (data_mem[i] != i) {
    //         *test_failed += 1;
    //     }else{
    //         *test_passed += 1;
    //     }
    // }
    return 0;
}