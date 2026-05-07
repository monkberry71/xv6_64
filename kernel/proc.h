#pragma once

#include <stdint.h>
#include "vm.h"
#include "x86_64.h"
#include "file.h"

enum proc_state { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
};

struct proc {
    uint64_t sz;
    pte_t *pml4;
    char *kstack;
    enum proc_state state;
    uint64_t pid;
    struct proc *parent;
    struct trap_frame *tf;
    struct regi_pile *rp; // for syscall
    struct context *context;
    void* chan;
    uint32_t killed;
    // struct fil
    struct inode *cwd;
    char name[32];
};

void swtch(struct context* *old, struct context *new_p);
struct proc* myproc(void);
void kthread_init(void* thread_func);
void process_init(void);
void scheduler(void);
void yield(void);
void sched(void);
void user_init(void);
uint64_t fork(void);
void wakeup(void* chan);
void sleep(void* chan, struct spin_lock *lk);