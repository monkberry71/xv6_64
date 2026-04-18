#include <stdint.h>
#include "driver/uart.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"

// bump, only use in main.
static uint64_t bump_ptr;
static int bump_enable;

static void bump_init(void) {
    bump_enable = 1;
    extern char _kernel_phys_end[];
    bump_ptr = (uint64_t) _kernel_phys_end;
    // we aligned it at the linker script, dont worry about it
}

uint64_t bump_alloc_page_4kb(void) {
    if(!bump_enable) return 0;
    uint64_t p = bump_ptr;
    bump_ptr += PGSIZE_4KB;
    memset((void*)P2V_KERN(p), 0, PGSIZE_4KB);
    return p;
}

int main(uint32_t mb2_info_phys) {
    // uint32_t* test_writing_point = KERN_BASE + 8;
    // *test_writing_point = 0xDEADBEEF;
    // test ===================

    init_serial();
    serial_puts("Hello ");

    bump_init();
    uint32_t* bump_test_ptr = P2V_KERN((void*)bump_alloc_page_4kb());
    memcpy(bump_test_ptr, "DEADBEEF", 9);
    serial_puts((void*)bump_test_ptr);

    for(;;);
}