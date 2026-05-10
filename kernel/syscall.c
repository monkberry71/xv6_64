#include <stdint.h>
#include "x86_64.h"
#include "mmu.h"
#include "syscall.h"
#include "driver/uart.h"
#include "proc.h"
#include "gop.h"

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

    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry); // entry point setting

    wrmsr(MSR_FMASK, FL_IF);
}

extern int64_t sys_chdir(void);
extern int64_t sys_close(void);
extern int64_t sys_dup(void);
// extern int64_t sys_exec(void);
extern int64_t sys_exit(void);
extern int64_t sys_fork(void);
extern int64_t sys_fstat(void);
// extern int64_t sys_getpid(void);
// extern int64_t sys_kill(void);
// extern int64_t sys_link(void);
extern int64_t sys_mkdir(void);
extern int64_t sys_mknod(void);
extern int64_t sys_open(void);
// extern int64_t sys_pipe(void);
extern int64_t sys_read(void);
// extern int64_t sys_sbrk(void);
// extern int64_t sys_sleep(void);
// extern int64_t sys_unlink(void);
extern int64_t sys_wait(void);
extern int64_t sys_write(void);
// extern int64_t sys_uptime(void);
extern int64_t sys_draw(void);

typedef int64_t (*syscall_func) (void);
static syscall_func syscalls[] = {
    [SYS_fork]    =sys_fork,
    [SYS_exit]    =sys_exit,
    [SYS_wait]    =sys_wait,
    // [SYS_pipe]    =sys_pipe,
    [SYS_read]    =sys_read,
    // [SYS_kill]    =sys_kill,
    // [SYS_exec]    =sys_exec,
    [SYS_fstat]   =sys_fstat,
    [SYS_chdir]   =sys_chdir,
    [SYS_dup]     =sys_dup,
    // [SYS_getpid]  =sys_getpid,
    // [SYS_sbrk]    =sys_sbrk,
    // [SYS_sleep]   =sys_sleep,
    // [SYS_uptime]  =sys_uptime,
    [SYS_open]    =sys_open,
    [SYS_write]   =sys_write,
    [SYS_mknod]   =sys_mknod,
    // [SYS_unlink]  =sys_unlink,
    // [SYS_link]    =sys_link,
    [SYS_mkdir]   =sys_mkdir,
    [SYS_close]   =sys_close,
    [SYS_draw]    =sys_draw
};

void syscall_dispatch(struct regi_pile *rp) {
    // dont use tf interrupt parts
    uint64_t num = rp->rax;
    myproc()->rp = rp;

    #define NELEM(x) (sizeof(x)/sizeof((x)[0]))
    if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        rp->rax = syscalls[num]();
    } else {
        serial_hex(num);
        serial_puts("<- unknown syscall\n");
        rp->rax = -1;
    }
}

int64_t sys_draw(void) {
    struct regi_pile *rp = myproc()->rp;
    gop_draw_rect(rp->rdi, rp->rsi, rp->rdx, rp->r10, rp->r8);
    return 0;
}