#include <stdint.h>
#include "spinlock.h"
#include "cpu.h"
#include "params.h"
#include "string.h"
#include "mmu.h"
#include "kalloc.h"

struct {
    struct proc procs[NPROC];
} ptable;

int next_pid = 1;

struct proc* myproc(void) {
    struct cpu *c;
    struct proc *p;

    push_cli();
    c = mycpu();
    p = c->proc;
    pop_cli();
}

void fork_ret(void) {
    // -------{context rip=fork_ret} trap_ret ------ trap_frame ---- stack_bottom
    static int first = 1;
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
    for(int i=0; i<NPROC; i++) {
        struct proc *p = &ptable.procs[i];
        if(p->state == UNUSED) {
            p->state = EMBRYO;
            p->pid = next_pid++;

            // release lock

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
}

void kthread_init(void* thread_func) {
    struct proc *p = alloc_proc();
    p->sz = PGSIZE_4KB;
    memset(p->tf, 0, sizeof(struct trap_frame));
    p->tf->cs = (SEG_KCODE << 3);
    p->tf->ss = (SEG_KDATA << 3);
    p->tf->rflags = FL_IF;
    p->tf->rsp = p->kstack + KSTACKSIZE;
    p->tf->rip = thread_func;

    p->state = RUNNABLE;
}
