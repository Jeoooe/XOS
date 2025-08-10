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
extern gdt_ptr

code_selector equ (1<<3)
data_selector equ (2<<3)

section .text
global _start
_start:
    push ebx    ; ards_count
    push eax    ; magic

    call console_init

    xchg bx, bx
    call gdt_init
    xchg bx, bx

    lgdt [gdt_ptr]
    jmp dword code_selector:_next ;加载选择子

_next:
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call memory_init
    xchg bx, bx

    mov esp, 0x10000
    xchg bx, bx
    call kernel_init

    ; xchg bx, bx
    ; mov eax, 0
    ; int 0x80
    
    jmp $