#include <stdint.h>
#include "cpu.h"
#include "x86_64.h"

struct cpu cpus[NCPU];

static void per_cpu_init(struct cpu* c) {
    c->self = c;
    wrmsr(MSR_GS_BASE, (uint64_t) c); // entry kernel codes before user space entry will use this
    // after user space, we dont assure that gs base will be our cpu struct, but it is very likely since our
    // user space code won't change it
    wrmsr(MSR_KERNEL_GS_BASE, (uint64_t) c); // gs must be this "c", if we are in kernel code
}

void cpu_init() {
    // for now, just bsp
    per_cpu_init(&cpus[0]);
}