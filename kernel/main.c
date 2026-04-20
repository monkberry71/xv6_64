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
    kinit();
    // kalloc test
    uint32_t *test = kalloc();
    memcpy(test, "DEADBEEF\n", 10);
    serial_puts((void*)test);

    char* p1 = kalloc();
    char* p2 = kalloc();
    if(p1 == p2) {
        panic("kalloc ??");
    }

    memset(p1, 0xAA, PGSIZE_4KB);
    if(*p1 != *(p1+PGSIZE_4KB-1)) {
        panic("kalloc ??");
    }
    memset(p2, 0xBB, PGSIZE_4KB);
    if(*p2 != *(p2+PGSIZE_4KB-1)) {
        panic("kalloc ??");
    }
    kfree(p1);
    void* p3 = kalloc();
    if(p1 != p3) {
        panic("kfree ???");
    }

    int count = 0;
    while(kalloc()) count++;
    serial_hex(count);
    
    

    // direct mapping test
    // uint32_t* test_writing_point = (void*)(PAGE_OFFSET + 8);
    // memcpy(test_writing_point, "DEADBEEF", 9);
    // serial_puts((void*)(KERN_BASE+8));

    // serial_hex((uint64_t)reserved_mb2_info->total_size);
    // serial_puts("\n");
    // uint32_t* bump_test_ptr = bump_alloc_page_4kb();
    // memcpy(bump_test_ptr, "DEADBEEF", 9);
    // serial_puts((void*)bump_test_ptr);


    serial_puts("Bye\n");
    for(;;);
}