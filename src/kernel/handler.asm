[bits 32]

extern handler_table

section .text   ;代码段
%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
; xchg bx, bx
%ifn %2
    push 0x20222202
%endif
    push %1
    jmp interrupt_entry
%endmacro

interrupt_entry:

    push ds
    push es
    push fs
    push gs
    pusha

    mov eax, [esp + 12 * 4]

    push eax
    
    ; 调用中断函数
    call [handler_table + eax * 4]

global interrupt_exit
interrupt_exit:
    
    add esp, 4
    
    popa
    pop gs
    pop fs
    pop es
    pop ds

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
INTERRUPT_HANDLER 0x1a, 0
INTERRUPT_HANDLER 0x1b, 0
INTERRUPT_HANDLER 0x1c, 0
INTERRUPT_HANDLER 0x1d, 0
INTERRUPT_HANDLER 0x1e, 0
INTERRUPT_HANDLER 0x1f, 0

INTERRUPT_HANDLER 0x20, 0
INTERRUPT_HANDLER 0x21, 0
INTERRUPT_HANDLER 0x22, 0
INTERRUPT_HANDLER 0x23, 0
INTERRUPT_HANDLER 0x24, 0
INTERRUPT_HANDLER 0x25, 0
INTERRUPT_HANDLER 0x26, 0
INTERRUPT_HANDLER 0x27, 0
INTERRUPT_HANDLER 0x28, 0
INTERRUPT_HANDLER 0x29, 0
INTERRUPT_HANDLER 0x2a, 0
INTERRUPT_HANDLER 0x2b, 0
INTERRUPT_HANDLER 0x2c, 0
INTERRUPT_HANDLER 0x2d, 0
INTERRUPT_HANDLER 0x2e, 0
INTERRUPT_HANDLER 0x2f, 0

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
    dd interrupt_handler_0x1a
    dd interrupt_handler_0x1b
    dd interrupt_handler_0x1c
    dd interrupt_handler_0x1d
    dd interrupt_handler_0x1e
    dd interrupt_handler_0x1f
    dd interrupt_handler_0x20
    dd interrupt_handler_0x21
    dd interrupt_handler_0x22
    dd interrupt_handler_0x23
    dd interrupt_handler_0x24
    dd interrupt_handler_0x25
    dd interrupt_handler_0x26
    dd interrupt_handler_0x27
    dd interrupt_handler_0x28
    dd interrupt_handler_0x29
    dd interrupt_handler_0x2a
    dd interrupt_handler_0x2b
    dd interrupt_handler_0x2c
    dd interrupt_handler_0x2d
    dd interrupt_handler_0x2e
    dd interrupt_handler_0x2f

section .text

extern syscall_check
extern syscall_table
global syscall_handler
syscall_handler:
    xchg bx, bx

    ;验证中断号
    push eax
    call syscall_check
    add esp, 4

    push 0x20222202

    push 0x80 

    push ds
    push es
    push fs
    push gs
    pusha

    ;向中断处理函数传递中断向量
    push 0x80 
    xchg bx, bx
    
    push edx
    push ecx
    push ebx
    
    ; 调用 系统调用 处理函数
    call [syscall_table + eax * 4]

    xchg bx, bx
    add esp, 12 ;恢复栈

    ;设置系统调用返回值
    mov dword [esp + 8 * 4], eax

    jmp interrupt_exit
