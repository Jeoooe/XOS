[bits 32]

magic   equ 0xe85250d6
i386    equ 0
length  equ header_end - header_start

section .multiboot2
header_start:
    dd magic
    dd i386
    dd length
    dd -(magic + i386 + length)

    ;结束标记
    dw 0
    dw 0
    dd 8
header_end:

extern console_init
extern kernel_init
extern memory_init
extern gdt_init

section .text
global _start
_start:
    push ebx    ; ards_count
    push eax    ; magic

    call console_init
    call gdt_init
    call memory_init
    call kernel_init

    ; xchg bx, bx
    ; mov eax, 0
    ; int 0x80
    
    jmp $