#include <stdint.h>
// Volatile to prevent compiler optimizations
volatile uint32_t result_1 = 0;
volatile uint32_t result_2 = 0;
volatile uint32_t result_3 = 0;

int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    result_1 = fib(4);
    result_2 = fib(6);
    result_3 = fib(12);
    return 0;
}