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
#include "spinlock.h"
#include "x86_64.h"
#include "proc.h"
#include "driver/uart.h"
#include "fs.h"
static int map_pages(pte_t *pml4, void *va, uint64_t size, uint64_t pa, uint64_t perm);
static pte_t *kpml4 = 0;
pte_t* get_kpml4(void) {
    return kpml4;
}

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

// xv6 uses setup_kvm for #1. kpgdir init #2. user process vm init
// we need to separate it to two funcs because 1. We can't use bump alloc after kpml4 init 
// 2. we need to copy from kpml4, not making new direct mapping and kernel mapping.
pte_t* setup_uvm(void) {
    pte_t* pml4;

    pml4 = (pte_t*) kalloc();
    if(pml4 == 0) return 0;
    memset(pml4, 0, PGSIZE_4KB); // kalloc doesnt 0-fill a page.

    for(int i = 256; i < 512; i++) {
        pml4[i] = kpml4[i];
    }

    return pml4;
}

// Load the code into address 0 of pml4
void init_uvm(pte_t *pml4, char *init, uint64_t sz) {
    if(sz >= PGSIZE_4KB) {
        panic("init is too big");
    }

    char *mem = kalloc();
    memset(mem, 0, PGSIZE_4KB);
    map_pages(pml4, 0, PGSIZE_4KB, V2P(mem), PTE_W | PTE_U);
    memcpy(mem, init, sz);
}

void switch_kvm(void) {
    wcr3(V2P_KERN(kpml4));
}

void switch_uvm(struct proc *p) {
    if(p == 0) {
        panic("switch_uvm: no proc");
    }
    if(p->kstack == 0) {
        panic("switch+uvm: no kstack");
    }
    if(p->pml4 == 0) {
        panic("switch_uvm: no pml4");
    }

    push_cli();
    mycpu()->ts.rsp[0] = (uint64_t) p->kstack + KSTACKSIZE; // for interrupt
    mycpu()->kernel_stack = (uint64_t) p->kstack + KSTACKSIZE; // for syscall
    wcr3(V2P(p->pml4)); // user_init uses kalloc
    pop_cli();
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
    // end must be the last page's starting point
    // so if va + size is already aligned, end will be next
    // page's starting addr, which is redundant

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

uint64_t dealloc_uvm(pte_t *pml4, uint64_t old_sz, uint64_t new_sz) {
    if(new_sz >= old_sz) {
        return old_sz;
    }

    for(uint64_t addr = ROUNDUP(new_sz, PGSIZE_4KB); addr < old_sz; addr += PGSIZE_4KB) {
        pte_t *pte = walk_pml4(pml4, (void*) addr, 0);
        if(!pte) {
            // We actually don't know which(PML4, PDPT, PD) one is NULL, so just be safe
            addr = (addr + PGSIZE_2MB) & ~(PGSIZE_2MB - 1); // jump 2MB
            addr -= PGSIZE_4KB; // loop compensate
            continue;
        }

        if((*pte & PTE_P) != 0) {
            uint64_t pa = PTE_ADDR(*pte);
            if(pa == 0) {
                panic("dealloc_uvm : can't free");
            }

            kfree(P2V(pa));
            *pte = 0;
        }
    }
    return new_sz;
}

// allocate new pml4 and phy mem to grow process
uint64_t alloc_uvm(pte_t *pml4, uint64_t old_sz, uint64_t new_sz) {
    if(new_sz < old_sz) {
        // ???
        return old_sz;
    }

    if(new_sz >= USER_TOP) {
        // 256 kernel pml4 entries
        return 0;
    }

    for(uint64_t addr = ROUNDUP(old_sz, PGSIZE_4KB); addr < new_sz; addr += PGSIZE_4KB) {
        void* mem = kalloc();
        if(!mem) {
            serial_puts("alloc_uvm OOM\n");
            dealloc_uvm(pml4, new_sz, old_sz);
            return 0;
        }
        memset(mem, 0, PGSIZE_4KB);
        if(map_pages(pml4, (void*) addr, PGSIZE_4KB, V2P(mem), PTE_W | PTE_U) < 0) {
            serial_puts("alloc_uvm OOM\n");
            dealloc_uvm(pml4, new_sz, old_sz);
            kfree(mem);
            return 0;
        }
    }
    return new_sz;
}

// free recursively, table must be virtual
static void free_entry(pte_t *table, int lv) {
    if(lv == 3) {
        // lv3 -> it is a pt, all these entries are already freed by deallov_uvm
        // free itself
        kfree((void*) table);
        return;
    }
    
    int max = (lv == 0) ? 256 : 512; 
    // lv0 -> pml4, we need to keep the upper 256 entries, they are kernel's.
    for(int i=0; i<max; i++) {
        if(table[i] & PTE_P) {
            free_entry(P2V(PTE_ADDR(table[i])), lv+1);
        }
    }
    kfree((void*) table);
    return;
}

// free a page tables itselves
// sz is old sz
void free_vm(pte_t *pml4, uint64_t sz) {
    if(pml4 == 0) {
        panic("free_vm: no pml4");
    }
    // dealloc_uvm(pml4, USER_TOP, 0); <<<< very slow
    dealloc_uvm(pml4, sz, 0);

    // lv = 0 pml4
    // lv = 1 pdpt
    // lv = 2 pd
    // lv = 3 pt
    free_entry(pml4, 0);
}

pte_t* copy_uvm(pte_t *pml4, uint64_t sz) {
    pte_t *new_pml4 = setup_uvm();
    if(new_pml4 == 0) return 0;

    uint64_t addr;
    // leave the first paage unmapped as null page
    for(addr = PGSIZE_4KB; addr < sz; addr += PGSIZE_4KB) {
        pte_t *pte = walk_pml4(pml4, (void*) addr, 0);
        if(pte == 0) {
            panic("copy_uvm: pte should exist");
        }
        if(!(*pte & PTE_P)) {
            panic("copy_uvm: page not present");
        }
        uint64_t pa = PTE_ADDR(*pte);
        uint64_t flags = PTE_FLAGS(*pte);
        
        void* mem = kalloc();
        if(mem == 0) {
            goto bad;
        }
        memcpy(mem, P2V(pa), PGSIZE_4KB);
        if(map_pages(new_pml4, (void*) addr, PGSIZE_4KB, V2P(mem), flags) < 0) {
            kfree(mem);
            goto bad;
        }
    }
    return new_pml4;
    bad:
        free_vm(new_pml4, addr);
        return 0;
}

int load_uvm(pte_t *pml4, char *addr, struct inode *ip, uint64_t offset, uint64_t sz) {
    if( (uint64_t) addr % PGSIZE_4KB != 0) {
        panic("load_uvm : addr must be 4kb aligned");
    }

    for(int i=0; i < sz; i+=PGSIZE_4KB) {
        pte_t *pte = walk_pml4(pml4, addr+i, 0);
        if(pte == 0) {
            panic("load_uvm : pte doesnt exist");
        }

        uint64_t pa = PTE_ADDR(*pte);

        uint64_t how_many_to_read = (sz-i) < PGSIZE_4KB ? (sz - i) : PGSIZE_4KB;

        uint64_t how_many_read = readi(ip, P2V(pa), offset+i, how_many_to_read);
        if(how_many_read != how_many_to_read) return -1;
    }
    return 0;
}

void clear_pte_u(pte_t *pml4, char *uva) {
    pte_t *pte = walk_pml4(pml4, uva, 0);

    if(pte == 0) {
        panic("clear_pte_u: clear what?");
    }

    *pte &= ~PTE_U;
}

char* uva2dma(pte_t *pml4, char *uva) {
    // user virtual addr to direct mapping addr
    pte_t *pte = walk_pml4(pml4, uva, 0);
    if(!pte) return 0;
    if(!(*pte & PTE_P)) return 0;
    if(!(*pte & PTE_U)) return 0;

    return (void*)P2V(PTE_ADDR(*pte));
}

// copy len bytes from p to user virtual addr in pml4
// use this when the pml4 is not on your cr3, we can emulate writing
int copy_out(pte_t *pml4, uint64_t va, void* p, uint64_t len) {
    char *buf = p;
    while(len > 0) {
        uint64_t va0 = ROUNDDOWN(va, PGSIZE_4KB);
        uint64_t pa0 = (uint64_t) uva2dma(pml4, (void*)va0);
        if(!pa0) return -1;

        // va0 --- va ---- (va0 + 4kb)

        uint64_t to_write = PGSIZE_4KB - (va - va0);
        to_write = (to_write > len) ? len : to_write;

        // va0 --- va ---- (va0 + 4kb)
        //          <- to_write ->

        memcpy((void*)(pa0 + (va - va0)), buf, to_write);
        len -= to_write;
        buf += to_write;
        va = va0 + PGSIZE_4KB;
    }
    return 0;
    // uint64_t va0 = ROUNDDOWN(va, PGSIZE_4KB);
    // uint64_t dma0 = uva2dma(pml4, (void*)va0);
    // uint64_t offset = va - va0;

    // for(; va0 < len; va0 += )
}

