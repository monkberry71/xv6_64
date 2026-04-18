#pragma once

#define KERN_PHYS_BASE 0x100000
#define PAGE_OFFSET 0xFFFF888000000000ULL
#define KERN_BASE 0xFFFFFFFF80000000ULL

// Direct map trans
#define V2P(v) ((uint64_t)(v) - PAGE_OFFSET)
#define P2V(p) ((void*)((uint64_t)(p) + PAGE_OFFSET))

#define V2P_KERN(v) ((uint64_t)(v) - KERN_BASE + KERN_PHYS_BASE)
#define P2V_KERN(p) ((void*)((uint64_t)(p) - KERN_PHYS_BASE + KERN_BASE))