ENTRY(_start)
KERNBASE = 0xFFFFFFFF80000000;

SECTIONS {
    . = 0x100000;

    /* boot sections */
    .boot ALIGN(4096) : {
        KEEP(*(.multiboot2))
        *(.text.boot)
        *(.rodata.boot)
    }    

    .bss.boot ALIGN(4096) (NOLOAD) : {
        *(.bss.boot)
    }

    /* kernel sections, 
    VMA = KERNBASE + offset
    LMA = offset
    After boot.asm jumps to `main`, all code runs at high VMA
    */

    . += KERNBASE;

    /* 
    AT will set our Load addr
    GRUB sees it and load there,
    but our codes will have high address derefs
    */

    .text ALIGN(4096) : AT(ADDR(.text) - KERNBASE) {
        *(.text*)
    }

    .rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNBASE) {
        *(.rodata*)
    }

    .data ALIGN(4096) : AT(ADDR(.data) - KERNBASE) {
        *(.data*)
    }

    .bss ALIGN(4096) (NOLOAD) : AT(ADDR(.bss) - KERNBASE) {
        *(COMMON)
        *(.bss*)
    }

    . = ALIGN(4096);
    _end = .; 
    /* use V2P(end) if you need it plz*/

}