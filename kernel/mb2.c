#include <stdint.h>
#include "mb2.h"
#include "mmu.h"
#include "memlayout.h"
#include "bump.h"
#include "string.h"

struct mb2_info* reserved_mb2_info;

void preserve_mb2(uint32_t mb2_info_phys) {
    struct mb2_info *mb2_info_given = P2V_KERN(mb2_info_phys);
    reserved_mb2_info = bump_alloc_page_4kb();
    for(uint32_t alloc = PGSIZE_4KB; alloc < mb2_info_given->total_size; alloc += PGSIZE_4KB) {
        bump_alloc_page_4kb();
    }

    memcpy(reserved_mb2_info, mb2_info_given, mb2_info_given->total_size);
}