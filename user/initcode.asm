format elf64

section ".text" executable

public _start
_start:
    mov rax, 1
    syscall

    test rax, rax
    jz .child

.parent:
    mov rax, 22
    mov rdi, 300
    mov rsi, 300
    mov rdx, 50
    mov r10, 50
    mov r8, 0x00FFFF00
    syscall

    jmp .loop

.child:
    mov rax, 22
    mov rdi, 0
    mov rsi, 0
    mov rdx, 30
    mov r10, 30
    mov r8, 0x00FFFF

.loop:
    jmp .loop
