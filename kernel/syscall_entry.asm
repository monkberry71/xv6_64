format elf64
macro pushaq {
    push rax ;14
    push rbx ;13
    push rcx ;12
    push rdx ;11
    push rsi ;10
    push rdi ;9
    push rbp ;8
    ; rept 8 n:8 
    ; {
    ;     push r#n
    ; } 
    ; why?
    push r8 ;7
    push r9 ;6
    push r10 ;5
    push r11 ;4
    push r12 ;3
    push r13 ;2
    push r14 ;1
    push r15 ;0
}

macro popaq {
    ; rept 8 n:8 
    ; {
    ;     reverse
    ;     pop r#n
    ;     display `n
    ; }
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
}

section ".text"
public syscall_entry
extrn syscall_dispatch
syscall_entry:
use64
    swapgs
    mov [gs:8], rsp ; save our user_rsp to mycpu struct
    mov rsp, [gs:16] ; get kstack from mycpu struct

    pushaq

    mov rdi, rsp
    call syscall_dispatch
    ; stack -->
    ; r15 ... r10 ... rax
    mov [rsp + 14 * 8], rax; return val to rax

    popaq
    mov rsp, [gs:8] ; user stack 
    swapgs
    sysretq; sysret becomes sysretl, which returns to 32 bit mode, use sysretq