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
