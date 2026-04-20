format elf64

KERN_BASE = 0xFFFFFFFF80000000

section '.multiboot2' align 8
MB2_MAGIC = 0xE85250D6
MB2_ARCH = 0

macro GDT_desc limit, base, access, flags {
    dw limit
    dw base and 0xFFFF
    db (base shr 16) and 0xFF
    db access
    db flags
    db (base shr 24) and 0xFF
}

PDE_P = 1 shl 0 ; present
PDE_RW = 1 shl 1 ; writable
PDE_US = 1 shl 2 ; User
PDE_PWT = 1 shl 3 ; write-through
PDE_PCD = 1 shl 4; cache disable
PDE_A = 1 shl 5 ; accessed
PDE_PS = 1 shl 7; page size 
PDE_XD = 1 shl 63; no exe

macro pde_t addr, flags {
    dq addr + flags
}

mb2_header:
    dd MB2_MAGIC ; magic num
    dd MB2_ARCH ; architecture
    dd mb2_header_end - mb2_header ; size
    dd -(MB2_MAGIC + MB2_ARCH + (mb2_header_end - mb2_header)) ; hashing
    dw 0 ; end tag type
    dw 0 ; end tag flags
    dd 8 ; ent tag size
mb2_header_end:

section '.text.boot' executable

public _start
_start:
use32
    cli
    mov edi, ebx; GRUB gives us multiboot2 info struct pointer, keep it.

    ; we need to make a page mapping VA [0,1GB) -> PA [0,1GB) and VA[KERN_BASE, KERN_BASE+1GB) -> PA[0,1GB)
    ; to make a seamless jumping

    ; enable PAE, which makes 64bit addr possible
    mov eax, cr4
    or eax, 1 shl 5; 5th bit is PAE;
    or eax, 1 shl 7; PGE
    mov cr4, eax

    ; cr3 reg holds the pml4 addr, it needs to be physical addr tho.
    mov eax, entry_pml4
    mov cr3, eax

    ; set EFER long mode enable, to start a real long mode
    mov ecx, 0xC0000080; the addr of EFER, in msr
    rdmsr
    or eax, 1 shl 8; LAE
    wrmsr

    ; set paging on
    mov eax, cr0
    or eax, 1 shl 31
    mov cr0, eax

    lgdt [gdtr_ptr]
    jmp 8:long_mode_entry
use64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, 0
    mov fs, ax
    mov gs, ax

    mov rsp, stack_top + KERN_BASE
    
    extrn main
    mov rax, main
    jmp rax

.halt:
    hlt
    jmp .halt
section '.rodata.boot' align 4096
gdt:
    dq 0; null descriptor
    
    ; ; 64bit code section desc
    ; dw 0xFFFF ; limit
    ; dw 0x0000 ; base
    ; db 0x00; base
    ; db 0x9A; access 1001 1010, present=1, DPL=0, S=1, Type=(code, executable, readable)
    ; db 0xAF; flags 1010, G=1, D=0, L=1, AVL=0
    ; db 0x00; base

    ; ;64bit data section desc
    ; dw 0xFFFF
    ; dw 0x0000
    ; db 0x00
    ; db 0x92 ;1001 0010, present=1, DPL=0, S=1, Type=(data, readable, writable)
    ; db 0xAF
    ; db 0x00;
    GDT_desc 0xFFFF, 0x000000, 0x9A, 0xAF
    ; access 0x9A == 1001 1010, Present=1, DPL=00, S=1, Type (code, exec, read)
    ; flags 0xAF == 1010, G=1, D=0, Longmode=1, AVL(Reservedo)=0
    GDT_desc 0xFFFF, 0x000000, 0x92, 0xAF
    ; access 0x92 == 1001 0010, Present=1, DPL=00, S=1, Type (data, r/w)
gdt_end:

gdtr_ptr:
    dw gdt_end - gdt - 1
    dd gdt

align 4096
entry_pml4:
    pde_t p3_ident, PDE_P or PDE_RW ; [0]
    times 510 dq 0
    pde_t p3_kernel, PDE_P or PDE_RW ; [511]
    

align 4096
p3_ident:
    pde_t 0, PDE_P or PDE_RW or PDE_PS; [0,1G] -> [0,1G]
    times 511 dq 0 ; fill

align 4096
p3_kernel:
    times 510 dq 0; index 0~509
    pde_t 0, PDE_P or PDE_RW or PDE_PS; index 510
    dq 0 ; index 511

section '.bss.boot' align 4096
stack_bottom:
rb 4096 * 4
stack_top: