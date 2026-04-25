#pragma once
#include <stdint.h>
#include "seg.h"
#include "params.h"
#include "proc.h"

struct cpu {
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
    return &cpus[0];
};