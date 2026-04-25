#pragma once

// rflags
#define FL_IF (1<<9)

#define PGSIZE_4KB 4096
#define PGSIZE_2MB (2 * 1024 * 1024) 
#define PGSIZE_1GB (1ULL * 1024 * 1024 * 1024)

#define ROUNDUP(sz, size) (((sz) + (size) - 1) & ~((uint64_t)(size) -1))
#define ROUNDDOWN(sz, size) ((sz) & ~((uint64_t)(size) - 1))

#define PML4_IDX(va) (((uint64_t)(va) >> 39) & 0x1FF)
#define PDPT_IDX(va)  (((uint64_t)(va) >> 30) & 0x1FF)
#define PD_IDX(va)    (((uint64_t)(va) >> 21) & 0x1FF)
#define PT_IDX(va)    (((uint64_t)(va) >> 12) & 0x1FF)

// PDE_P = 1 shl 0 ; present
// PDE_RW = 1 shl 1 ; writable
// PDE_US = 1 shl 2 ; User
// PDE_PWT = 1 shl 3 ; write-through
// PDE_PCD = 1 shl 4; cache disable
// PDE_A = 1 shl 5 ; accessed
// PDE_PS = 1 shl 7; page size 
// PDE_XD = 1 shl 63; no 

#define PTE_P 0x001 
#define PTE_W 0x002
#define PTE_U 0x004
#define PTE_PWT 0x008
#define PTE_PCD 0x010
#define PTE_A 0x020
#define PTE_D 0x040
#define PTE_PS 0x080
#define PTE_G 0x100
#define PTE_XD (1ULL << 63)

#define K_FLAGS (PTE_P | PTE_G | PTE_W)

struct gate_desc {
    uint16_t off_15_0;
    uint16_t cs;
    uint8_t ist;
    uint8_t type;
    uint16_t off_mid;
    uint32_t off_hi;
    uint32_t reserved;
} __attribute__((packed));

#define IDT_TYPE_INT 0x0e
#define IDT_TYPE_TRAP 0x0f

#define SETGATE(gate, istrap, m_ist, sel, off, dpl) \
{\
    (gate).off_15_0 = (uint64_t)(off) & 0xFFFF; \
    (gate).cs = (uint16_t)(sel); \
    (gate).ist = (uint8_t)(m_ist); \
    (gate).type = 0x80 | ((dpl) << 5) | ((istrap) ? 0xF : 0xE); \
    (gate).off_mid = ((uint64_t)(off) >> 16) & 0xFFFF; \
    (gate).off_hi = ((uint64_t)(off)>> 32) & 0xFFFFFFFF; \
    (gate).reserved = 0; \
}
