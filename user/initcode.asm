format elf64

section ".text" executable

public _start
_start:
    mov rax, 22
    mov rdi, 300
    mov rsi, 300
    mov rdx, 50
    mov r10, 50
    mov r8, 0x00FFFF00
    syscall

    jmp _start