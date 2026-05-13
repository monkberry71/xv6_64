format elf64

section ".text" executable

public _start
_start:
    ; mov rax, 15
    ; mov rdi, console_path
    ; mov rsi, 1
    ; syscall

    ; mov rdi, rax
    ; mov rax, 16
    ; mov rsi, message
    ; mov rdx, message_len
    ; syscall

    mov rax, 7
    mov rdi, init_path
    mov rsi, argv
    syscall


;     mov rax, 1
;     syscall ;fork

;     test rax, rax
;     jz .child

; .parent:
;     mov rax, 22
;     mov rdi, 300
;     mov rsi, 300
;     mov rdx, 50
;     mov r10, 50
;     mov r8, 0xFF0000
;     ;; draw(300, 300, 50, 50, red)
;     syscall

;     mov rax, 3 ; wait
;     syscall

;     mov rax, 22         
;     mov rdi, 300
;     mov rsi, 300
;     mov rdx, 50
;     mov r10, 50
;     mov r8, 0x00FF00    
;     syscall ;; draw(300,300,50,50,G)
;     jmp .loop

; .child:
;     mov rax, 22
;     mov rdi, 0
;     mov rsi, 0
;     mov rdx, 30
;     mov r10, 30
;     mov r8, 0x00FFFF
;     ;; draw(0, 0, 30, 30, sky)
;     syscall 

;     mov rcx, 0xFFFFFFFF
; .delay_loop:
;     dec rcx
;     jnz .delay_loop

;     mov rax, 2
;     syscall; exit


.loop:
    jmp .loop

section ".rodata"
; console_path db "/console", 0
; message db "write syscall to console works", 10
; message_len = $ - message
init_path db "/init",0
arg0 db "init",0

align 8
argv dq arg0, 0
