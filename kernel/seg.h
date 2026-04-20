#include <stdint.h>
#define SEG_KCODE 1
#define SEG_KDATA 2 
#define SEG_UDATA32 3 // place holder
#define SEG_UDATA 4 // syscall...
#define SEG_UCODE 5
#define SEG_TSS 6   
#define NSEGS 8

#define LONG_MODE_SEG_DESC(access, flag) \
    (((uint64_t)(access) << 40) | ((uint64_t)((flag) & 0xFF) << 48))
    // limit and base ignored
    // why flag & 0xFF ==> I want my flags to be 1 byte, always 0xAF or 0xCF?

//https://wiki.osdev.org/Task_State_Segment
struct task_state {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;
} __attribute__((packed));

