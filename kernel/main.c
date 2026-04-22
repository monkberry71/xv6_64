#include <stdint.h>
#include "driver/uart.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "bump.h"
#include "mb2.h"
#include "vm.h"
#include "kalloc.h"
#include "debug.h"
#include "trap.h"

// void stof() {
//     stof();
// }

// extern struct mb2_info* reserved_mb2_info;

int main(uint32_t mb2_info_phys) {
    // uint32_t* test_writing_point = KERN_BASE + 8;
    // *test_writing_point = 0xDEADBEEF;
    // test ===================

    init_serial();
    serial_puts("Hello\n");

    bump_init();
    preserve_mb2(mb2_info_phys);
    kvmalloc();
    serial_puts("kinit...");
    kinit();
    serial_puts("done\n");
    seg_init();
    tv_init();
    idt_init();

    uint32_t *test = io_remap(0x8, 16);
    *test = 0xDEADBEEF;
    
    // __asm__ volatile("ud2");
    // __asm__ volatile("mov $0x8000, %%rsp\n\tud2" ::: "memory");

    // stof();
    // seg _init test
    // char buf[10] = {'D','E','A','D','B','E','A','F','\n', 0};
    // serial_puts(buf);

    serial_puts("Bye\n");
    for(;;);
}