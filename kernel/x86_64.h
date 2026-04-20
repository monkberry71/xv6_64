#include <stdint.h>

static inline void wcr3(uint64_t val) {
    // load(write) cr3
    __asm__ volatile("movq %0,%%cr3" : : "r" (val));
}