#pragma once
#include <stdint.h>

struct spin_lock {
    uint64_t locked;

    // dbg
    char *name;
    struct cpu *cpu; // who holding the lock?
    // uint64_t pcs[10]; do we need it?
};
void push_cli(void);
void pop_cli(void);
void release(struct spin_lock *lk);
void acquire(struct spin_lock *lk);
void init_lock(struct spin_lock *lk, char *name);
uint64_t holding(struct spin_lock *lk);
