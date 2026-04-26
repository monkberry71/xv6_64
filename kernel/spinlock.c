#include <stdint.h>
#include "spinlock.h"
#include "x86_64.h"
#include "mmu.h"
#include "cpu.h"
#include "debug.h"
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

void init_lock(struct spin_lock *lk, char *name) {
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

uint64_t holding(struct spin_lock *lk) {
    uint64_t r;
    push_cli();
    r = lk->locked && lk->cpu == mycpu();
    pop_cli();
    return r;
}

void acquire(struct spin_lock *lk) {
    push_cli(); 
    // if a lock is used by an int handler, that lock must not be used with
    // int enabled.
    // so, just assume every each lock is used by int handler.
    if(holding(lk)) {
        panic("acquire again");
    }

    while(xchg(&lk->locked, 1) != 0);
    // while xchg(lock, 1) == 1

    __sync_synchronize(); // mfence?

    lk->cpu = mycpu();
    // get_caller_pcs(&lk, lk->pcs);
    // stack --->
    // return_addr --- saved_rbp --- acquire's_lk
}

void release(struct spin_lock *lk) {
    if(!holding(lk)) {
        panic("release what?");
    }

    lk->cpu = 0;

    __sync_synchronize(); // mfence?

    __asm__ volatile("movq $0, %0" : "=m"(lk->locked): );

    pop_cli();
}