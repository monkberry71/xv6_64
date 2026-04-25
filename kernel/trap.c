#include <stdint.h>
#include "mmu.h"
#include "seg.h"
#include "x86_64.h"
#include "debug.h"
#include "driver/uart.h"
#include "trap.h"
#include "lapic.h"
#include "gop.h"

struct gate_desc idt[256];
extern uint64_t vectors[256];

void tv_init(void) {
    for(int i=0; i < 256; i++) {
        SETGATE(idt[i], 1, 0, SEG_KCODE << 3, vectors[i], 0);
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
        serial_hex(fault_addr);
        serial_putc('\n');
    }
    if(tf->trap_no == T_IRQ0 + IRQ_TIMER) {

        lapic_eoi();
        return;
    }
    if(tf->trap_no == T_DBLFLT) {
        gop_draw_rect(0,0,500,500, GOP_RED);
        panic("DF DF DF");
    }
}