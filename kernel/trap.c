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
    // serial_hex(tf->trap_no);
    // serial_putc('\n');
    // if(tf->cs == ((SEG_UCODE << 3) | DPL_USER ) && tf->rip < 0x1000) {
    // serial_printf("low user rip rip=%x rsp=%x rbp=%x rax=%x\n",
    //     tf->rip, tf->rsp, tf->rbp, tf->rax);
    // for(;;);
    // }
    if(tf->trap_no == T_PGFLT) {
        uint64_t fault_addr;
        __asm__ volatile("movq %%cr2, %0" : "=r"(fault_addr));

        if(tf->cs & DPL_USER) {
            serial_printf("pid=%x %s SEGFAULT va=%x rip=%x err=%x\n", myproc()->pid, myproc()->name, fault_addr, tf->rip, tf->err);
            exit();
        }
        
        serial_printf("kernel page fault va=%x rip=%x err=%x\n", fault_addr, tf->rip, tf->err);
        panic("kernel page fault");
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

    // kill check
    if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER) exit();

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

    // kill check
    if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER) exit();
}