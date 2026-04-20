#include <stdint.h>
#include "vm.h"
#include "memlayout.h"
#include "mmu.h"
#include "bump.h"
#include "debug.h"
#include "x86_64.h"
#include "cpu.h"
#include "params.h"

static pde_t *kpml4 = 0;

char __attribute__((aligned(16))) ist0[KSTACKSIZE];

static void set_tss_desc(uint64_t *gdt_slot, void* tss_base, uint32_t limit, char flags, char access) {
    uint64_t base = (uint64_t) tss_base;
    uint64_t low = 0, high = 0;
    low |= (uint64_t)(limit & 0xFFFF); // limit[15:0] byte2
    low |= (base & 0xFFFFFF) << 16; // base [15:0] byte5
    low |= ((uint64_t)access << 40); // access byte6
    low |= (uint64_t)((limit >> 16) & 0xF) << 48; // limit[19:16]
    low |= (uint64_t)(flags & 0xF) << 52; // flags[3:0] 
    low |= (uint64_t)((base >> 24) & 0xFF) << 56; // base[31:24]

    high |= (base >> 32) & 0xFFFFFFFFULL; // base[63:32]

    gdt_slot[0] = low;
    gdt_slot[1] = high;
}

void seg_init(void) {
    struct cpu* c;
    c = mycpu(); // ok why xv6 derefs the cpus array?
    c->gdt[SEG_KCODE] = LONG_MODE_SEG_DESC(0x9A, 0xAF);
    c->gdt[SEG_KDATA] = LONG_MODE_SEG_DESC(0x92, 0xCF);
    c->gdt[SEG_UDATA32] = LONG_MODE_SEG_DESC(0,0);
    c->gdt[SEG_UDATA] = LONG_MODE_SEG_DESC(0xF2, 0xCF);
    c->gdt[SEG_UCODE] = LONG_MODE_SEG_DESC(0xFA, 0xAF);
    
    // we need a IST for a double fault
    memset(&c->ts, 0, sizeof(c->ts)); // TSS itself is per cpu
    c->ts.ist[0] = (uint64_t)(ist0 + sizeof(ist0));
    c->ts.iopb = sizeof(c->ts); // 

    set_tss_desc(&c->gdt[SEG_TSS], &c->ts, sizeof(c->ts) - 1, 0, 0x89);

    // kcode and kdata have same indices as before, no need to reload segment selector

    wgdt(c->gdt, sizeof(c->gdt));
    wtr(SEG_TSS << 3);
}

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