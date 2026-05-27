#include <stdint.h>
#include "mmu.h"
#include "seg.h"
#include "x86_64.h"
#include "debug.h"
#include "driver/uart.h"
#include "trap.h"
#include "lapic.h"
#include "gop.h"
#include "proc.h"
#include "driver/kbd.h"

struct gate_desc idt[256];
extern uint64_t vectors[256];

void tv_init(void) {
    for(int i=0; i < 256; i++) {
        SETGATE(idt[i], 0, 0, SEG_KCODE << 3, vectors[i], 0);
    }
    SETGATE(idt[8], 1, 1, SEG_KCODE << 3, vectors[8], 0);
    // SETGATE(idt[6], 1, 1, SEG_KCODE << 3, vectors[6], 0);
}

void idt_init(void) {
    widt(idt, sizeof(idt));
}

void trap(struct trap_frame *tf) {
    serial_hex(tf->trap_no);
    serial_putc('\n');
    if(tf->trap_no == T_PGFLT) {
        uint64_t fault_addr;
        __asm__ volatile("movq %%cr2, %0" : "=r"(fault_addr));
    serial_puts("pgflt cr2=");
    serial_hex(fault_addr);
    serial_puts(" rip=");
    serial_hex(tf->rip);
    serial_puts(" err=");
    serial_hex(tf->err);
    serial_puts(" cs=");
    serial_hex(tf->cs);
    serial_puts(" rsp=");
    serial_hex(tf->rsp);

    serial_puts(" rax="); serial_hex(tf->rax);
serial_puts(" rbx="); serial_hex(tf->rbx);
serial_puts(" rcx="); serial_hex(tf->rcx);
serial_puts(" rbp="); serial_hex(tf->rbp);
serial_puts(" rdi="); serial_hex(tf->rdi);
serial_puts(" rsi="); serial_hex(tf->rsi);
    serial_putc('\n');
    }
    if(tf->trap_no == T_IRQ0 + IRQ_TIMER) {

        lapic_eoi();
    }
    if(tf->trap_no == T_IRQ0 + IRQ_KBD) {
        kbd_intr();
        lapic_eoi();
    }
    if(tf->trap_no == T_DBLFLT) {
        gop_draw_rect(0,0,500,500, GOP_RED);
        panic("DF DF DF");
    }

    // give up 
    if(
        myproc() && // not scheduler
        myproc()->state == RUNNING && 
        // yield will set it RUNNABLE, so it shouldve been running, otherwise
        // sleeping thread will be marked RUNNABLE
        // sleeping thread will yield itself fine
        tf->trap_no == T_IRQ0 + IRQ_TIMER
    ) {
        yield();
    }
}