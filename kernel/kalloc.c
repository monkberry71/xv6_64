#include <stdint.h>
#include "bump.h"
#include "mb2.h"
#include "mmu.h"
#include "debug.h"
#include "memlayout.h"
#include "string.h"

struct {
    struct run *free_list;
} kmem;

struct run {
    struct run *next;
};

void kfree(char* v) {
    // v must be direct mapping
    struct run *r;
    uint64_t dmap_bump_end = P2V(p_bump_end());
    if((uint64_t)v % PGSIZE_4KB || v < dmap_bump_end || V2P(v) >= PHY_STOP) {
        panic("kfree frees wrong addr");
    }
    memset(v, 1, PGSIZE_4KB);

    // possible locking
    r = (void*)v;
    r->next = kmem.free_list;
    kmem.free_list = r;
    // possible locking

}

void free_range(void *vstart, void* vend) {
    // it should be a direct mapping address
    char *p;
    p = (char*) ROUNDUP((uint64_t)vstart, PGSIZE_4KB);
    // vstart | space | p
    for(; p + PGSIZE_4KB <= (char*)vend; p+=PGSIZE_4KB) {
        kfree(p);
    }
    // last p || space || vend
}

extern struct mb2_info* reserved_mb2_info;
void kinit(void) {
    kmem.free_list = 0;
    struct mb2_tag* tag;
    MB2_FOREACH_TAG(reserved_mb2_info, tag) {
        if(tag->type != MB2_TAG_MMAP) continue;
        
        struct mb2_tag_mm *mm_tag = tag;
        uint64_t entry_length = (mm_tag->tag.size - sizeof(struct mb2_tag_mm)) / mm_tag->entry_size;
        for(uint64_t i=0; i<entry_length; i++) {
            struct mb2_mmap_entry *e = (struct mb2_mmap_entry*)((char*)mm_tag->entries + i * mm_tag->entry_size);
            if(e->type != MB2_MMAP_AVAIL) continue;

            // below bump is not free-able.
            uint64_t mmap_start = e->base_addr;
            uint64_t mmap_end = e->base_addr + e->length;
            if(mmap_start < p_bump_end()) {
                mmap_start = p_bump_end();
            }
            
            free_range(P2V(mmap_start), P2V(mmap_end));
        }
    }
    bump_off();
}

void* kalloc(void) {
    struct run* r;

    // possible locking
    r = kmem.free_list;
    if(r) {
        kmem.free_list = r->next;
        memset(r, 2, PGSIZE_4KB); // use-before-init catch
    }
    // possible locking
    return r;
}