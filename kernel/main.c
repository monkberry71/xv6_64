#include <stdint.h>
#include "driver/uart.h"
#define KERN_BASE 0xFFFFFFFF80000000UL
int main(void) {
    // uint32_t* test_writing_point = KERN_BASE + 8;
    // *test_writing_point = 0xDEADBEEF;
    // test ===================

    init_serial();
    serial_puts("Hello");
    for(;;);
}