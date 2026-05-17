#include <stdint.h>
#include "spinlock.h"
#include "cpu.h"
#include "params.h"
#include "string.h"
#include "mmu.h"
#include "kalloc.h"
#include "debug.h"
#include "memlayout.h"
#include "fs.h"

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
struct proc* alloc_kthread(void) {

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


            // uint8_t *sp = (uint8_t*)new_stack + KSTACKSIZE;
            // sp -= sizeof(struct trap_frame);
            // p->tf = (void*) sp;
            // ------>
            // ------------ trap_frame --- stack_bottom
            //              ^sp

            // sp -= 8;
            // *(uint64_t *) sp = (uint64_t) trap_ret;
            //  -------->
            // -------- trap_ret -- trap_frame --- stack_bottom
            
            // sp -= sizeof(struct context);
            // p->context = (void*) sp;
            // memset(p->context, 0, sizeof(struct context));
            // -------context ----- trap_ret ------ trap_frame ---- stack_bottom
            // p->context->rip = (uint64_t) ;
            // -- {context rip = fork_ret} ---- trap_ret --- tf -- st_bottom

            return p;

        }
    }
    release(&ptable.lock);
    return 0; // failed
}
void kthread_init(void* thread_func) {
    struct proc *p = alloc_kthread();

    // p->pml4 = P2V(V2P_KERN(get_kpml4()));
    uint64_t kpml4_phy = V2P_KERN(get_kpml4());
    p->pml4 = P2V(kpml4_phy); // pml4 entry should be direct mapping
    p->sz = PGSIZE_4KB;
    // memset(p->tf, 0, sizeof(struct trap_frame));
    // p->tf->cs = (SEG_KCODE << 3);
    // p->tf->ss = (SEG_KDATA << 3);
    // p->tf->rflags = FL_IF;
    // p->tf->rsp = (uint64_t)(p->kstack + KSTACKSIZE);
    // p->tf->rip = (uint64_t)thread_func;
    char* sp = p->kstack + KSTACKSIZE;
    sp -= sizeof(struct context);
    p->context = (void*) sp;
    memset(p->context, 0, sizeof(struct context));
    p->context->rip = (uint64_t) thread_func;


    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
}

void syscall_ret(void);
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
            sp -= sizeof(struct regi_pile);
            p->rp = (void*) sp;
            // st ->
            // --- regi_pile --- stack_bottom

            sp -= 8;
            *(uint64_t *) sp = (uint64_t) syscall_ret;
            // st ->
            // --- syscall_ret --- rp --- st_btm
            
            sp -= sizeof(struct context);
            p->context = (void*) sp;
            memset(p->context, 0, sizeof(struct context));
            // st ->
            // --- ctxt --- sys_ret --- rp --- st_btm
            p->context->rip = (uint64_t) fork_ret;
            // st ->
            // --- {ctxt rip = &fork_ret} --- sys_ret --- rp --- st_btm

            return p;

        }
    }
    release(&ptable.lock);
    return 0; // failed
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
    // memset(p->tf, 0, sizeof(struct trap_frame));
    // p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    // p->tf->ss = (SEG_UDATA << 3) | DPL_USER;
    // p->tf->rflags = FL_IF;
    // p->tf->rsp = (uint64_t) PGSIZE_4KB;
    // p->tf->rip = 0; // initcode.asm begins
    memset(p->rp, 0, sizeof(struct regi_pile));
    p->rp->r11 = (uint64_t) FL_IF; // rflags
    p->rp->rcx = (uint64_t) 0;
    p->rp->rsp = (uint64_t) PGSIZE_4KB;

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

void sleep(void* chan, struct spin_lock *lk) {
    struct proc *p = myproc();

    if(p == 0) {
        panic("sleep in scheduler");
    }

    if(lk == 0) {
        // we need a lock to protect our sleepin condition
        // if wakeup come in between check and sleep,
        // the proc may sleep forever, so we lock it before check
        panic("sleep with out lk");
    }

    // We must acquire ptable.lock to change p->state.
    // After we hold ptable.lock, we won't miss any wakeup,
    // because wakeup runs with ptable.lock locked.
    // so we don't need to protect the sleeping condition.
    // thus we can releae the lk. and we must, since the sleeping cond
    // should be changed anyway.

    // if lk is ptable.lock, we need it locked to change the p state anyway
    // and it will be unlocked with scheduler, so we dont need to worry
    // about the sleeping condition won't budge or not.
    if(lk != &ptable.lock) {
        acquire(&ptable.lock);
        release(lk);
    }

    p->chan = chan;
    p->state = SLEEPING;

    sched();

    p->chan = 0; // cleanup

    if(lk != &ptable.lock) {
        release(&ptable.lock);
        acquire(lk);
    }
}

/*
wake up all processes sleeping on the chan, pure version
use this when already ptable lock is held.
*/
static void wakeup_pure(void* chan) {
    for(int i=0; i<NPROC; i++) {
        struct proc *p = &ptable.procs[i];
        if(p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE; // wake up
        }
    }
}

void wakeup(void* chan) {
    acquire(&ptable.lock);
    wakeup_pure(chan);
    release(&ptable.lock);
}

int64_t fork(void) {
    struct proc *new_p = alloc_proc();
    struct proc *cur_p = myproc();

    if(new_p == 0) {
        return -1;
    }

    // new_p->pml4 = copy
    new_p->pml4 = copy_uvm(cur_p->pml4, cur_p->sz);
    if(new_p->pml4 == 0) {
        kfree(new_p->kstack);
        new_p->kstack = 0;
        new_p->state = UNUSED;
        return -1;
    }

    new_p->sz = cur_p->sz;
    new_p->parent = cur_p;
    *(new_p->rp) = *(cur_p->rp); // copy the value of the rp
    new_p->rp->rax = 0; // return value must be zero.

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! open file dup
    for(int i=0; i < NOFILE; i++) {
        if(cur_p->ofile[i])
            new_p->ofile[i] = file_dup(cur_p->ofile[i]);
    }
    new_p->cwd = idup(cur_p->cwd);

    safe_strcpy(new_p->name, cur_p->name, sizeof(cur_p->name));
    
    uint64_t pid = new_p->pid;
    acquire(&ptable.lock);
    new_p->state = RUNNABLE;
    release(&ptable.lock);

    return pid;
}

int64_t wait(void) {
    struct proc *curp = myproc();

    acquire(&ptable.lock);

    uint64_t pid;
    for(;;) {
        int have_kids = 0;
        for(int i=0; i<NPROC; i++) {
            struct proc *p = &ptable.procs[i];
            if(p->parent != curp) continue;
            have_kids = 1;
            if(p->state == ZOMBIE) {
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;

                free_vm(p->pml4, p->sz);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;

                release(&ptable.lock);
                return pid;
            }
        }

        if(!have_kids || curp->killed) {
            // no kids
            // or dead
            release(&ptable.lock);
            return -1;
        }

        sleep(curp, &ptable.lock);
    }

    // it is just a condition variable while loop, but the condition is too big, we use for
}

void exit(void) {
    struct proc *curp = myproc();
    if(curp == init_proc) {
        panic("exit: init??");
    }

    // !!!!!!!!!close all open file
    for(int fd=0; fd < NOFILE; fd++) {
        if(curp->ofile[fd]) {
            file_close(curp->ofile[fd]);
            curp->ofile[fd] = 0;
        }
    }

    if(curp->cwd) {
        iput(curp->cwd);
        curp->cwd = 0;
    }

    acquire(&ptable.lock);

    // tell my parent I love them very much
    wakeup_pure(curp->parent);

    // I am going to die, please adopt my children
    for(int i=0; i<NPROC; i++) {
        struct proc *p = &ptable.procs[i];
        if(p->parent == curp) {
            p->parent = init_proc;
            if(p->state == ZOMBIE) {
                wakeup_pure(init_proc);
            }
        }
    }

    // good bye cruel world
    curp->state = ZOMBIE;
    sched();
    panic("I am dead");
}