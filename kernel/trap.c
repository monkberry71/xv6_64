#include <stdint.h>
#include "mmu.h"
#include "seg.h"
#include "x86_64.h"
#include "debug.h"
#include "driver/uart.h"

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
    if(tf->trap_no == 8) {
        panic("DF DF DF");
    }
}