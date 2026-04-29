#include <stdint.h>
#include <x86_64.h>
#include <mmu.h>

void syscall_entry(void);
void syscall_init(void) {
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= EFER_SCE; // SCE : system call enable
    wrmsr(MSR_EFER, efer);

    uint64_t star = (0x0018ULL << 48) | (0x0008ULL << 32);
    // our gdt : 0: SEG_NULL 8: SEG_KCODE 16: SEG_KUSER 24: DUMMY_SEG_UCODE 32: SEG_UDATA 40: SEG_UCODE
    //           
    // STAR : 0x 0018 0008 XXXX XXXX
    //             24   8    NO   NO
    //           RET  CALL
    // CALL is 8 -> when syscall, cs = gdt[X], ss = gdt[X+8] = gdt[16]
    // RET is 24 -> when sysret, cs = gdt[X+16] = gdt[40], ss = gdt[X+8] = gdt[32]
    wrmsr(MSR_STAR, star);

    wrmsr(MSR_LSTAR, syscall_entry); // entry point setting

    wrmsr(MSR_FMASK, FL_IF);
    
}