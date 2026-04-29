#pragma once
#include <stdint.h>
#include "seg.h"
#include "params.h"
#include "proc.h"

struct cpu {
    struct cpu* self;
    uint64_t user_rsp;
    uint64_t kernel_stack;
    uint8_t lapic_id;
    struct context* scheduler;
    uint64_t gdt[NSEGS];
    struct task_state ts;
    int ncli;
    int int_ena;
    struct proc *proc;
};

extern struct cpu cpus[NCPU];
static inline struct cpu *mycpu(void) {
    // return &cpus[0];
    struct cpu* c;
    __asm__ volatile("movq %%gs:0, %0" : "=r"(c));
    return c;
};
void cpu_init(void);