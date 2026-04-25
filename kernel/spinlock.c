#include <stdint.h>
#include "x86_64.h"
#include "mmu.h"
#include "cpu.h"
void push_cli(void) {
    uint64_t rflags = read_rflags();
    // maybe it was already disabled when ncli == 0
    // ex) interrupt handlers automatically disable it

    cli();
    if(mycpu()->ncli == 0) {
        mycpu()->int_ena = rflags & FL_IF;
    }
    mycpu()->ncli += 1;
}

void pop_cli(void) {
    if (read_rflags() & FL_IF) {
        // already enabled??
        panic("popcli - Interrupt is already enabled");
    }

    if(--mycpu()->ncli < 0) {
        panic("popcli - ncli negative");
    }

    if(mycpu()->ncli == 0 && mycpu()->int_ena) {
        sti();
    }
}