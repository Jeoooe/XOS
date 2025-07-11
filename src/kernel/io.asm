[bits 32]
;输入输出函数实现

section .text ;代码段

global inb ;导出inb
inb:
    ;栈帧
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp + 8]
    in al, dx;端口号dx 8 bit 输入 ax
    
    ;进行一点延迟
    jmp $+2
    jmp $+2
    jmp $+2

    leave
    ret

global outb ;导出inb
outb:
    ;栈帧
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]  ;port
    mov eax, [ebp + 12] ;value
    out dx, al
    
    ;进行一点延迟
    jmp $+2
    jmp $+2
    jmp $+2

    leave
    ret

global inw ;导出inb
inw:
    ;栈帧
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp + 8]
    in ax, dx;端口号dx 8 bit 输入 ax
    
    ;进行一点延迟
    jmp $+2
    jmp $+2
    jmp $+2

    leave
    ret

global outw ;导出inb
outw:
    ;栈帧
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]  ;port
    mov eax, [ebp + 12] ;value
    out dx, ax
    
    ;进行一点延迟
    jmp $+2
    jmp $+2
    jmp $+2

    leave
    ret