format elf64

macro pushaq {
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    ; rept 8 n:8 
    ; {
    ;     push r#n
    ; } 
    ; why?
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
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

extrn trap
section ".text"
use64
public all_traps
public trap_ret
all_traps:
    ; push ds
    ; push es
    ; push fs
    ; push gs
    ; stack -->
    ; num / err / rip / cs / rflags / ori_rsp / ss 
    ; ^cur_rsp
    test qword [rsp + 24], 3; cs's user-ness check
    jz .from_kernel ; if it is 0, no swapgs
    swapgs
.from_kernel:
    pushaq
    ; xv6 set ds and es to kernel code segment, maybe because of int system call? we dont need it perhaps

    mov rdi, rsp
    call trap
trap_ret:
    popaq
    ; pop gs
    ; pop fs
    ; pop es
    ; pop ds 
    add rsp, 16; err and vector num

    ; rip / cs / rflags / ori_rsp / ss
    ; ^cur_rsp
    test qword [rsp + 8], 3
    jz .restore_all
    swapgs
.restore_all:
    iretq


