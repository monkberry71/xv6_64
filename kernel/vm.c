#include <stdint.h>
#include "vm.h"
#include "memlayout.h"
#include "mmu.h"
#include "bump.h"
#include "debug.h"
#include "x86_64.h"
#include "cpu.h"
#include "params.h"
#include "string.h"
#include "kalloc.h"

static pte_t *kpml4 = 0;

char __attribute__((aligned(16))) ist0[KSTACKSIZE];

static void set_tss_desc(uint64_t *gdt_slot, void* tss_base, uint32_t limit, uint8_t flags, uint8_t access) {
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

pte_t* setup_kvm(void) {
    pte_t* pml4;
    
    pml4 = (pte_t*) bump_alloc_page_4kb();
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

static pte_t *walk_pml4(pte_t *pml4, const void *va, int alloc) {
    // bit offset 
    // PT 12~21; >> 12
    // PD 21~30; >> 21
    // PDPT 30~39; >> 30
    // PML4 39~48; >> 39
    // 39 30 21 12
    // 
    pte_t *entry_tables = pml4;
    for(int i=0; i<4; i++) {
        int shift = 39 - i * 9;
        pte_t *entry_pointer = &entry_tables[((uint64_t)va >> shift) & 0x1FF];
        if(i == 3) {
            // ok, now the *entry itself is on the last level, we must return it or 0
            // return (!alloc && *entry_pointer & PTE_P) ? entry_pointer : 0;
            if(alloc) {
                return entry_pointer;
            }
            return (*entry_pointer & PTE_P) ? entry_pointer : 0;
        }
        if(*entry_pointer & PTE_P) {
            if(*entry_pointer & PTE_PS) {
                return entry_pointer;
            }
            // no ps -> next level
            entry_tables = P2V((uint64_t)(*entry_pointer) & ~0xFFFULL); 
            // *entry_p will always have a physical addr.
            continue;
        }
        
        // no present bit? allocate or return 0
        if(!alloc || (entry_tables = (pte_t*) kalloc()) == 0) {
            return 0;
        }
        memset(entry_tables, 0, PGSIZE_4KB);
        *entry_pointer = V2P(entry_tables) | PTE_P | PTE_W | PTE_U;
        // entry_tables is always direct mapping, since it came from kalloc
    }
    return 0; // non-reachable
}

static int map_pages(pte_t *pml4, void *va, uint64_t size, uint64_t pa, uint64_t perm) {

    // pa must be aligned

    uint64_t large_start, end;
    // large_start --- va --- end --- va+size-1 --- end + 4096
    // large_start <= va <= end <= va+size-1 < end + 4096

    large_start = ROUNDDOWN((uint64_t)va, PGSIZE_4KB);
    end = ROUNDDOWN((uint64_t)va + size - 1, PGSIZE_4KB);

    for(uint64_t it = large_start;; it += PGSIZE_4KB, pa += PGSIZE_4KB) {
        pte_t *pte_for_curr = walk_pml4(pml4, (void*)it ,1);
        if(pte_for_curr == 0) return -1;

        if(*pte_for_curr & PTE_P) {
            panic("remap");
        }

        *pte_for_curr = pa | perm | PTE_P;

        if (it == end) break; // break early for preventing overflow
    }
    return 0;
}

static uint64_t io_bump = IOREMAP_BASE;
void* io_remap(void* pa, uint64_t size) {
    uint64_t va_start = io_bump;
    uint64_t pa_offset = (uint64_t) pa & 0xFFF;
    uint64_t pa_paging_idx = ROUNDDOWN((uint64_t)pa, PGSIZE_4KB);

    int res = map_pages(kpml4, (void*)io_bump, size, pa_paging_idx, PTE_PCD | PTE_W);
    if(res != 0) {
        panic("io_remap failed by map_pages failure");
    }

    uint64_t end = ROUNDDOWN((uint64_t)io_bump + size -1, PGSIZE_4KB);
    io_bump = end + PGSIZE_4KB;
    return (void*)(va_start + pa_offset);

}


static void direct_map_init(pte_t* pml4) {
    if(pml4[PML4_IDX(PAGE_OFFSET)] != 0) {
        // why isn't it 0?
        panic("direct mapping init failed");
    }
    pte_t *pdpt = bump_alloc_page_4kb();
    pml4[PML4_IDX(PAGE_OFFSET)] =  V2P_KERN(pdpt) | K_FLAGS;
    // bump alloc's addrs are on the kernel mapping, so we need to use V2P_KERN
    
    // I think max ram 128G is enough...
    for(int i=0; i<128; i++) {
        uint64_t phy_addr = i * PGSIZE_1GB;
        pte_t pdpt_entry = phy_addr | PTE_PS | K_FLAGS;
        pdpt[PDPT_IDX(PAGE_OFFSET) + i] = pdpt_entry;
    }
}

static void kernel_map_init(pte_t *pml4) {
    if(pml4[PML4_IDX(KERN_BASE)] != 0) {
        //why isn't it 0?
        panic("kernel mapping init failed");
    }
    pte_t *pdpt = bump_alloc_page_4kb();
    pml4[PML4_IDX(KERN_BASE)] = V2P_KERN(pdpt) | K_FLAGS;

    for(int i=0; i<2; i++) {
        uint64_t phy_addr = i * PGSIZE_1GB;
        pte_t pdpt_entry = phy_addr | PTE_PS | K_FLAGS;
        pdpt[PDPT_IDX(KERN_BASE) + i] = pdpt_entry;
    }

}