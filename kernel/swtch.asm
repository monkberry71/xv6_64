format elf64

section ".text"
;  void swtch(struct context **old, struct context *new);
public swtch
swtch:
use64
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; stack ----->
    ; r15 r14 r13 r12 rbx rbp rip
    ; ^rsp

    mov [rdi], rsp ; *old = rsp
    mov rsp, rsi ; stack_pointer = new_stack_pointer
    ; A thread is consist of rsp and rip.
    ; rip will be same, so change the rsp

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

    
