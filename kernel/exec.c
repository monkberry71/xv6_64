#include <stdint.h>
#include "fs.h"
#include "file.h"
#include "driver/console.h"
#include "mmu.h"
#include "memlayout.h"
#include "vm.h"
#include "elf.h"
#include "params.h" 
#include "string.h"
#include "proc.h"

int64_t exec(char *path, char **argv) {
    struct inode *ip = namei(path);
    if(ip == 0) {
        cprintf("exec: failed");
        return -1;
    }

    ilock(ip);
    
    pte_t *pml4 = 0; // for bad 

    struct elf_header elf;
    int64_t res = readi(ip, (void*) &elf, 0, sizeof(struct elf_header));
    if(res != sizeof(struct elf_header))
        goto bad;

    // oh oh oh it's magic you know 
    if(elf.magic != ELF_MAGIC)
        goto bad;
    
    
    pml4 = setup_uvm();
    if(pml4 == 0) 
        goto bad;

    uint64_t sz = 0;
    struct prog_header *phs = (void*) elf.ph_off;
    for(short i = 0; i < elf.ph_num; i++) {
        uint64_t offset = (uint64_t) &phs[i];

        struct prog_header ph;
        int64_t res = readi(ip, (void*) &ph, offset, sizeof(struct prog_header));
        if(res != sizeof(struct prog_header)) 
            goto bad;
        
        if(ph.type != ELF_PROG_LOAD) {
            // only static loading
            continue;
        }

        // Must:
        // mem_sz(bytes the segment occupies in mem) >= file_sz (bytes the segment exists in the ELF file)
        // bss will be zero filled!!
        if(ph.mem_sz < ph.file_sz) 
            goto bad;

        // Overflow
        if(ph.vaddr + ph.mem_sz < ph.vaddr) 
            goto bad;
        
        // alloc uvm, ph.vaddr + ph.mem_sz is the end of the segment
        sz = alloc_uvm(pml4, sz, ph.vaddr + ph.mem_sz);
        if(sz == 0) 
            goto bad;
        
        // not aligned
        if(ph.vaddr % PGSIZE_4KB != 0)
            goto bad;

        if(load_uvm(pml4, (void*)ph.vaddr, ip, ph.offset, ph.file_sz) < 0) {
            goto bad;
        }

    }

    iunlock(ip);
    iput(ip);
    ip = 0; // cleanup?

    sz = ROUNDUP(sz, PGSIZE_4KB); // why?? alloc_uvm will do this wtf
    sz = alloc_uvm(pml4, sz, sz + 2 * PGSIZE_4KB);
    if(sz == 0) 
        goto bad;
    
    // make it canary
    clear_pte_u(pml4, (void*)(sz - 2 * PGSIZE_4KB)); 
    uint64_t stack_pointer = sz;

    int argc;
    uint64_t arg_ptrs[MAXARG+1];
    for(argc = 0; argv[argc]; argc++) {
        if(argc >= MAXARG)
            goto bad;
        
        stack_pointer -= strlen(argv[argc]) + 1;
        stack_pointer = ROUNDDOWN(stack_pointer, 8);
        if(copy_out(pml4, stack_pointer, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
        arg_ptrs[argc] = stack_pointer;

        
    }
    arg_ptrs[argc] = 0;
    
    stack_pointer -= (argc + 1) * 8; // +1 for NULL 
    stack_pointer -= 8; // argc

    int64_t argc_64 = argc;
    stack_pointer = ROUNDDOWN(stack_pointer, 16); // amd64 expects 16 byte alignment for a sp when jumping or calling 
    stack_pointer -= 8; // so if we directly enter the main func, it will expect the 16n+8 rsp because, call instruction will fill the return addr.
    
    if(copy_out(pml4, stack_pointer + 8, &argc_64, 8) < 0) 
        goto bad;

    if(copy_out(pml4, stack_pointer + 16, arg_ptrs, (argc + 1) * 8) < 0) 
        goto bad;


    // get the program name
    char *last, *s;
    for(last = s = path; *s; s++) {
        if(*s == '/') last = s+1;
    }

    struct proc *cur_p = myproc();
    safe_strcpy(cur_p->name, last, sizeof(cur_p->name));

    pte_t *old_pml4 = cur_p->pml4;
    uint64_t old_sz = cur_p->sz;
    cur_p->pml4 = pml4;
    cur_p->sz = sz;
    cur_p->rp->rcx = elf.entry; // main
    cur_p->rp->rsp = stack_pointer;
    cur_p->rp->r11 = FL_IF; // enable interupt in user
    cur_p->rp->rdi = argc;
    cur_p->rp->rsi = stack_pointer + 16; // argv addr

    switch_uvm(cur_p);
    free_vm(old_pml4, old_sz);
    return 0;
    // echo hi
    // st ->
    // argc - argv[0] - argv[1] - NULL - "hi\0" - "echo\0"

    bad:
        if(pml4) free_vm(pml4, sz);
        if(ip) {
            iunlock(ip);
            iput(ip);
        }
        return -1;
}