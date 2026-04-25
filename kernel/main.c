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
#include "gop.h"
#include "lapic.h"
#include "proc.h"
#include "params.h"

// void stof() {
//     stof();
// }

// extern struct mb2_info* reserved_mb2_info;
extern struct {
    struct proc procs[NPROC];
} ptable;

void proc_a(void) {
    // struct context* a_con;
    for(;;) {
        gop_draw_rect(300,300,50,50, 0);
        gop_draw_rect(300,300,50,50, GOP_BLU);
        swtch(&ptable.procs[0].context,ptable.procs[1].context); 
    }
}

void proc_b(void) {
    // struct context* b_con;
    for(;;) {
        gop_draw_rect(300,300,50,50, 0);
        gop_draw_rect(300,300,50,50, GOP_GRN);
        swtch(&ptable.procs[1].context,ptable.procs[0].context); 
    }
}

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
    gop_init();
    gop_draw_rect(0, 0, 100, 100, GOP_RED);   // red square
    gop_draw_rect(100, 0, 100, 100, GOP_GRN); // green square
    gop_draw_rect(200, 0, 100, 100, GOP_BLU); // blue square
    // __asm__ volatile("sti");
    lapic_init();

    kthread_init(proc_a);
    kthread_init(proc_b);

    // jump to proc_a
    // __asm__ volatile("mov %0, %rsp" : "m"ptable.procs[0]->)
    uint64_t* no_use;
    swtch(&no_use, (ptable.procs[0]).context);
    // uint32_t *test = io_remap(0x8, 16);
    // *test = 0xDEADBEEF;
    
    // __asm__ volatile("ud2");
    // __asm__ volatile("mov $0x8000, %%rsp\n\tud2" ::: "memory");

    // stof();
    // seg _init test
    // char buf[10] = {'D','E','A','D','B','E','A','F','\n', 0};
    // serial_puts(buf);

    serial_puts("Bye\n");
    // basic scheduler
    for(;;) {

    }
}