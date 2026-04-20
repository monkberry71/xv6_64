#include <stdint.h>
#include "driver/uart.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "bump.h"
#include "mb2.h"
#include "vm.h"

extern struct mb2_info* reserved_mb2_info;

int main(uint32_t mb2_info_phys) {
    // uint32_t* test_writing_point = KERN_BASE + 8;
    // *test_writing_point = 0xDEADBEEF;
    // test ===================

    init_serial();
    serial_puts("Hello\n");

    bump_init();
    preserve_mb2(mb2_info_phys);
    kvmalloc();

    // direct mapping test
    uint32_t* test_writing_point = (void*)(PAGE_OFFSET + 8);
    memcpy(test_writing_point, "DEADBEEF", 9);
    serial_puts((void*)(KERN_BASE+8));

    // serial_hex((uint64_t)reserved_mb2_info->total_size);
    // serial_puts("\n");
    // uint32_t* bump_test_ptr = bump_alloc_page_4kb();
    // memcpy(bump_test_ptr, "DEADBEEF", 9);
    // serial_puts((void*)bump_test_ptr);


    serial_puts("Bye\n");
    for(;;);
}