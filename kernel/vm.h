#pragma once
#include <stdint.h>

typedef uint64_t pde_t;

pde_t* setup_kvm(void);
void switch_kvm(void);
void kvmalloc(void);
static void direct_map_init(pde_t* pml4);
static void kernel_map_init(pde_t *pml4);