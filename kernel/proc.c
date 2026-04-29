#include <stdint.h>
#include "spinlock.h"
#include "cpu.h"
#include "params.h"
#include "string.h"
#include "mmu.h"
#include "kalloc.h"
#include "debug.h"
#include "memlayout.h"

struct {
    struct spin_lock lock;
    struct proc procs[NPROC];
} ptable;

int next_pid = 1;

struct proc* myproc(void) {
    push_cli();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_cli();
    return p;
}

void process_init(void) {
    init_lock(&ptable.lock, "ptable");
}

void fork_ret(void) {
    // -------{context rip=fork_ret} trap_ret ------ trap_frame ---- stack_bottom
    static int first = 1;

    // ptable lock from the scheduler
    release(&ptable.lock);
    if (first) {
        first = 0;
        // init some
    }
    // trap_ret ---- trap_frame --- stack_bottom
    // ^rsp (to return addr)
    return; // return to caller, trap_ret which we inserted, in the alloc_proc
}

// extern void* trap_ret; nonono
// 
// extern int x
// in a symbol table 
// x -> 0x00004 (addr of x)
// if I use a x in a code, it will deref it
// as same logic, 
// if I use a trap_ret in a code, it will deref it
// so declare it as a function or array
// &function_name == function_name in c
void trap_ret(void);
struct proc* alloc_proc(void) {

    acquire(&ptable.lock);
    for(int i=0; i<NPROC; i++) {
        struct proc *p = &ptable.procs[i];
        if(p->state == UNUSED) {
            p->state = EMBRYO;
            p->pid = next_pid++;

            release(&ptable.lock);

            void* new_stack = kalloc();
            if(!new_stack) {
                p->state = UNUSED;
                return 0;
            }
            p->kstack = new_stack;


            uint8_t *sp = (uint8_t*)new_stack + KSTACKSIZE;
            sp -= sizeof(struct trap_frame);
            p->tf = (void*) sp;
            // ------_>
            // ------------ trap_frame --- stack_bottom
            //              ^sp

            sp -= 8;
            *(uint64_t *) sp = (uint64_t) trap_ret;
            //  -------->
            // -------- trap_ret -- trap_frame --- stack_bottom
            
            sp -= sizeof(struct context);
            p->context = (void*) sp;
            memset(p->context, 0, sizeof(struct context));
            // -------context ----- trap_ret ------ trap_frame ---- stack_bottom
            p->context->rip = (uint64_t) fork_ret;
            // -- {context rip = fork_ret} ---- trap_ret --- tf -- st_bottom

            return p;

        }
    }
    release(&ptable.lock);
    return 0; // failed
}
void kthread_init(void* thread_func) {
    struct proc *p = alloc_proc();

    // p->pml4 = P2V(V2P_KERN(get_kpml4()));
    uint64_t kpml4_phy = V2P_KERN(get_kpml4());
    p->pml4 = P2V(kpml4_phy); // pml4 entry should be direct mapping
    p->sz = PGSIZE_4KB;
    memset(p->tf, 0, sizeof(struct trap_frame));
    p->tf->cs = (SEG_KCODE << 3);
    p->tf->ss = (SEG_KDATA << 3);
    p->tf->rflags = FL_IF;
    p->tf->rsp = (uint64_t)(p->kstack + KSTACKSIZE);
    p->tf->rip = (uint64_t)thread_func;

    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
}

static struct proc *init_proc;
void user_init(void) {
    extern char _binary_build_user_initcode_start[], _binary_build_user_initcode_size[];
    struct proc *p = alloc_proc();

    init_proc = p;
    p->pml4 = setup_uvm();
    if(p->pml4 == 0) {
        panic("user_init: setup_uvm failed");
    }
    init_uvm(p->pml4, _binary_build_user_initcode_start, (uint64_t) _binary_build_user_initcode_size);
    p->sz = PGSIZE_4KB;
    memset(p->tf, 0, sizeof(struct trap_frame));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ss = (SEG_UDATA << 3) | DPL_USER;
    p->tf->rflags = FL_IF;
    p->tf->rsp = (uint64_t) PGSIZE_4KB;
    p->tf->rip = 0; // initcode.asm begins

    safe_strcpy(p->name, "initcode", sizeof(p->name));
    // p->cwd

    // acquire forces above writes to be visible (sync_synchronize)
    // and assignment has to be atomic
    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
}

void scheduler(void) {
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;) {
        // enable int
        sti();

        acquire(&ptable.lock);
        for(int i=0; i<NPROC; i++) {
            struct proc *p = &ptable.procs[i];
            if(p->state != RUNNABLE) continue;

            c->proc = p;
            switch_uvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switch_kvm(); 
            // we don't switch to kvm, because all threads all kthread now
            c->proc = 0;
        }
        release(&ptable.lock);

    }
}

void sched(void) {
    struct proc *p = myproc();

    // checks :
    // A process that wants give up the CPU must:
    // 1. acquire process table lock
    // 2. release any other lock it is holding
    // 3. update its own state
    // 4. then call sched

    if(!holding(&ptable.lock)) {
        // 1. acquire process table lock
        panic("sched ptable.lock not holded");
    }
    
    if(mycpu()->ncli != 1) {
        // 2. release any other lock it is holding
        // push_cli is called in only 3 places
        // #1 myproc #2 switchuvm #3 spinlock #3.1 spinlock holding check
        // #1, #2, #3.1 would not call yield while interrupt diabled,
        // thus ncli == nlock
        panic("sched locks");
    }

    if(p->state == RUNNING) {
        // 3. update its own state
        panic("sched running");
    }

    if(read_rflags() & FL_IF) {
        // + since a lock is held, the interrupt must be disabled 
        // strictly
        panic("sched interruptible");
    }


    // need to store int_ena, because it is a property of a kthread.
    // proc->int_ena and proc->ncli would be appropriate, but 
    // that would break in the few places, when a lk is held but it is a pure kthread, not process

    int int_ena = mycpu()->int_ena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->int_ena = int_ena;
    
}

void yield(void) {
    acquire(&ptable.lock); // 1.
    myproc()->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}