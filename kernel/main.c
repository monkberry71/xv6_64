#include <stdint.h>
#include "driver/uart.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "bump.h"

int main(uint32_t mb2_info_phys) {
    // uint32_t* test_writing_point = KERN_BASE + 8;
    // *test_writing_point = 0xDEADBEEF;
    // test ===================

    init_serial();
    serial_puts("Hello ");

    bump_init();
    uint32_t* bump_test_ptr = bump_alloc_page_4kb();
    memcpy(bump_test_ptr, "DEADBEEF", 9);
    serial_puts((void*)bump_test_ptr);

    for(;;);
}