#pragma once
#include <stdint.h>

typedef uint64_t pte_t;

pte_t* setup_kvm(void);
void switch_kvm(void);
void kvmalloc(void);
static void direct_map_init(pte_t* pml4);
static void kernel_map_init(pte_t *pml4);
void seg_init(void);
void* io_remap(void* pa, uint64_t size);