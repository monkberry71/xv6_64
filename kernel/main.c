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
#include "driver/console.h"
#include "fs.h"

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

int64_t mknod_con(void);
int64_t file_write(struct file *f, char *addr, uint64_t n);

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
    lapic_init();
    process_init();
    user_init();
    syscall_init();
    rd_init();
    bcache_init();
    console_init();
    cprintf("--- Console Testing ---\n");
    // cprintf("Screen size: %d x %d\n", cons.max_cols, cons.max_rows);
    cprintf("Magic Num: 0x%x\n", 0xDEADBEAF);
    cprintf("Hello %s\n", "World from amd64");
    serial_puts("Bye\n");

    fs_init(ROOTDEV);
    mknod_con();
    // struct inode *rooti = namei("/");
    // if(rooti == 0) panic("wtf no root");
    // ilock(rooti);
    // cprintf("root type=%d size=%d\n", rooti->type, rooti->size);
    // iunlock(rooti);
    // iput(rooti);
    // basic scheduler

    
    scheduler();
}