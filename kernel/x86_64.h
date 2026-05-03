#pragma once
#include <stdint.h>

static inline void wcr3(uint64_t val) {
    // load(write) cr3
    __asm__ volatile("movq %0,%%cr3" : : "r" (val));
}

static inline void wgdt(void *p, uint64_t size) {
    volatile struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr = { size-1, (uint64_t) p};

    __asm__ volatile("lgdt %0" : : "m"(gdtr));
}

static inline void wtr(uint16_t selector) {
    __asm__ volatile("ltr %0" : : "r"(selector));
}

static inline void widt(void *p, uint64_t size) {
    volatile struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr = { size-1, (uint64_t) p};

    __asm__ volatile("lidt %0" : : "m"(gdtr));
}

// MSRs
#define MSR_EFER 0xC0000080 // we ve seen this at entry.asm, long mode enable
#define EFER_SCE 0x1 // system call enable
#define EFER_LME (1ULL << 8) // long mode enable

#define MSR_STAR 0xC0000081 // segment selector
#define MSR_LSTAR 0xC0000082 // rip 
#define MSR_FMASK 0xC0000084 // flag to mask

#define MSR_GS_BASE 0xC0000101 // gs base of this cpu
#define MSR_KERNEL_GS_BASE 0xC0000102 // gs base reserved for kernel mode


static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t) high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile("wrmsr" :: "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

static inline uint64_t read_rflags(void) {
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0" : "=r"(rflags));
    return rflags;
}

static inline void cli(void) {
    __asm__ volatile("cli");
}

static inline void sti(void) {
    __asm__ volatile("sti");
}

static inline uint64_t xchg(volatile uint64_t *addr, uint64_t new_val) {
    // read of *addr should not be optimized, it must read from memory all the time
    uint64_t res;

    __asm__ volatile(\
        "lock; xchgq %0, %1": // lock makes the next instruction 
        "+m"(*addr), "=a"(res) :
        "1"(new_val) :
        "cc"
    );
    return res;
}

struct regi_pile {    
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rsp;
};

struct trap_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t trap_no;
    uint64_t err;
    
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};