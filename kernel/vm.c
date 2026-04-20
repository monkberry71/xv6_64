#include <stdint.h>
#include "vm.h"
#include "memlayout.h"
#include "mmu.h"
#include "bump.h"
#include "debug.h"
#include "x86_64.h"
#include "vm.h"

static pde_t *kpml4 = 0;

// plan : 

// make a walkpml4 func
// make a mappage func


// make a direct mapping 
// make a kmalloc
// make a kinit

pde_t* setup_kvm(void) {
    pde_t* pml4;
    
    pml4 = (pde_t*) bump_alloc_page_4kb();
    if(pml4 == 0) return 0;

    // direct mapping
    direct_map_init(pml4);
    kernel_map_init(pml4);
    return pml4;
}

void switch_kvm(void) {
    wcr3(V2P_KERN(kpml4));
}

void kvmalloc(void) {
    kpml4 = setup_kvm();
    switch_kvm();
}


static void direct_map_init(pde_t* pml4) {
    if(pml4[PML4_IDX(PAGE_OFFSET)] != 0) {
        // why isn't it 0?
        panic("direct mapping init failed");
    }
    pde_t *pdpt = bump_alloc_page_4kb();
    pml4[PML4_IDX(PAGE_OFFSET)] =  V2P_KERN(pdpt) | K_FLAGS;
    // bump alloc's addrs are on the kernel mapping, so we need to use V2P_KERN
    
    // I think max ram 128G is enough...
    for(int i=0; i<128; i++) {
        uint64_t phy_addr = i * PGSIZE_1GB;
        pde_t pdpt_entry = phy_addr | PTE_PS | K_FLAGS;
        pdpt[PDPT_IDX(PAGE_OFFSET) + i] = pdpt_entry;
    }
}

static void kernel_map_init(pde_t *pml4) {
    if(pml4[PML4_IDX(KERN_BASE)] != 0) {
        //why isn't it 0?
        panic("kernel mapping init failed");
    }
    pde_t *pdpt = bump_alloc_page_4kb();
    pml4[PML4_IDX(KERN_BASE)] = V2P_KERN(pdpt) | K_FLAGS;

    for(int i=0; i<2; i++) {
        uint64_t phy_addr = i * PGSIZE_1GB;
        pde_t pdpt_entry = phy_addr | PTE_PS | K_FLAGS;
        pdpt[PDPT_IDX(KERN_BASE) + i] = pdpt_entry;
    }

}