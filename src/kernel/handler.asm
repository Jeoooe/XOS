[bits 32]

extern handler_table

section .text   ;代码段
%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
xchg bx, bx
%ifn %2
    push 0x20200202
%endif
    push %1
    jmp interrupt_entry
%endmacro

interrupt_entry:
    mov eax, [esp]
    call [handler_table + eax * 4]
    
    add esp, 8
    iret

INTERRUPT_HANDLER 0x00, 0
INTERRUPT_HANDLER 0x01, 0
INTERRUPT_HANDLER 0x02, 0
INTERRUPT_HANDLER 0x03, 0
INTERRUPT_HANDLER 0x04, 0
INTERRUPT_HANDLER 0x05, 0
INTERRUPT_HANDLER 0x06, 0
INTERRUPT_HANDLER 0x07, 0
INTERRUPT_HANDLER 0x08, 1
INTERRUPT_HANDLER 0x09, 0
INTERRUPT_HANDLER 0x0A, 1
INTERRUPT_HANDLER 0x0B, 1
INTERRUPT_HANDLER 0x0C, 1
INTERRUPT_HANDLER 0x0D, 1
INTERRUPT_HANDLER 0x0E, 1
INTERRUPT_HANDLER 0x0F, 0
INTERRUPT_HANDLER 0x10, 0
INTERRUPT_HANDLER 0x11, 1
INTERRUPT_HANDLER 0x12, 0
INTERRUPT_HANDLER 0x13, 0
INTERRUPT_HANDLER 0x14, 0
INTERRUPT_HANDLER 0x15, 1
INTERRUPT_HANDLER 0x16, 0
INTERRUPT_HANDLER 0x17, 0
INTERRUPT_HANDLER 0x18, 0
INTERRUPT_HANDLER 0x19, 0
INTERRUPT_HANDLER 0x1A, 0
INTERRUPT_HANDLER 0x1B, 0
INTERRUPT_HANDLER 0x1C, 0
INTERRUPT_HANDLER 0x1D, 0
INTERRUPT_HANDLER 0x1E, 0
INTERRUPT_HANDLER 0x1F, 0

section .data
global handler_entry_table
handler_entry_table:
    dd interrupt_handler_0x00
    dd interrupt_handler_0x01
    dd interrupt_handler_0x02
    dd interrupt_handler_0x03
    dd interrupt_handler_0x04
    dd interrupt_handler_0x05
    dd interrupt_handler_0x06
    dd interrupt_handler_0x07
    dd interrupt_handler_0x08
    dd interrupt_handler_0x09
    dd interrupt_handler_0x0A
    dd interrupt_handler_0x0B
    dd interrupt_handler_0x0C
    dd interrupt_handler_0x0D
    dd interrupt_handler_0x0E
    dd interrupt_handler_0x0F
    dd interrupt_handler_0x10
    dd interrupt_handler_0x11
    dd interrupt_handler_0x12
    dd interrupt_handler_0x13
    dd interrupt_handler_0x14
    dd interrupt_handler_0x15
    dd interrupt_handler_0x16
    dd interrupt_handler_0x17
    dd interrupt_handler_0x18
    dd interrupt_handler_0x19
    dd interrupt_handler_0x1A
    dd interrupt_handler_0x1B
    dd interrupt_handler_0x1C
    dd interrupt_handler_0x1D
    dd interrupt_handler_0x1E
    dd interrupt_handler_0x1F