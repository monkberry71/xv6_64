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
    
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};