format elf64

section ".text" executable

public _start
_start:
    mov rax, 1
    syscall ;fork

    test rax, rax
    jz .child

.parent:
    mov rax, 22
    mov rdi, 300
    mov rsi, 300
    mov rdx, 50
    mov r10, 50
    mov r8, 0xFF0000
    ;; draw(300, 300, 50, 50, red)
    syscall

    mov rax, 3 ; wait
    syscall

    mov rax, 22         
    mov rdi, 300
    mov rsi, 300
    mov rdx, 50
    mov r10, 50
    mov r8, 0x00FF00    
    syscall ;; draw(300,300,50,50,G)
    jmp .loop

.child:
    mov rax, 22
    mov rdi, 0
    mov rsi, 0
    mov rdx, 30
    mov r10, 30
    mov r8, 0x00FFFF
    ;; draw(0, 0, 30, 30, sky)
    syscall 

    mov rcx, 0x0FFFFFFF
.delay_loop:
    dec rcx
    jnz .delay_loop

    mov rax, 2
    syscall; exit


.loop:
    jmp .loop
