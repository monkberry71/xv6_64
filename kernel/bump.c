#include <stdint.h>
#include "mmu.h"
#include "memlayout.h"
#include "string.h"
#include "debug.h"

// bump, only use before kalloc
static uint64_t bump_ptr;
static int bump_enable;

void bump_init(void) {
    bump_enable = 1;
    extern char _kernel_phys_end[];
    bump_ptr = (uint64_t) _kernel_phys_end;
    // we ve aligned it at the linker script, dont worry about it
}

void* bump_alloc_page_4kb(void) {
    // it returns the kernel mapping virtual address
    if(!bump_enable) {
        panic("Bad bump usage");
    }
    uint64_t p = bump_ptr;
    bump_ptr += PGSIZE_4KB;
    memset((void*)P2V_KERN(p), 0, PGSIZE_4KB);
    return (void*)P2V_KERN(p);
}