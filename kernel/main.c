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
#include "cpu.h"
#include "syscall.h"
#include "bio.h"
#include "driver/ramdisk.h"

void proc_a(void) {
    // struct context* a_con;
    for(;;) {
        // gop_draw_rect(300,300,50,50, 0);
        gop_draw_rect(300,300,50,50, GOP_BLU);
        serial_putc('A');
        // swtch(&ptable.procs[0].context,ptable.procs[1].context); 
    }
}

void proc_b(void) {
    // struct context* b_con;
    for(;;) {
        // gop_draw_rect(300,300,50,50, 0);
        gop_draw_rect(300,300,50,50, GOP_GRN);
        serial_putc('B');
        // swtch(&ptable.procs[1].context,ptable.procs[0].context); 
    }
}

void proc_c(void) {
    // struct context* b_con;
    for(;;) {
        // gop_draw_rect(300,300,50,50, 0);
        gop_draw_rect(300,300,50,50, GOP_RED);
        serial_putc('C');
        // swtch(&ptable.procs[1].context,ptable.procs[0].context); 
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
    cpu_init();
    seg_init();
    tv_init();
    idt_init();
    gop_init();
    gop_draw_rect(0, 0, 100, 100, GOP_RED);   // red square
    gop_draw_rect(100, 0, 100, 100, GOP_GRN); // green square
    gop_draw_rect(200, 0, 100, 100, GOP_BLU); // blue square
    // __asm__ volatile("sti");
    lapic_init();
    process_init();
    

    // kthread_init(proc_b);
    // kthread_init(proc_c);
    user_init();
    // kthread_init(proc_a);
    syscall_init();

    rd_init();
    bcache_init();
    // test_bcache();

    serial_puts("Bye\n");
    // basic scheduler
    scheduler();
}