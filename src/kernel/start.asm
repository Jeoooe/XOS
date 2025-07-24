[bits 32]

extern console_init
extern kernel_init
extern memory_init

global _start
_start:
    push ebx    ; ards_count
    push eax    ; magic

    call console_init
    call memory_init
    ; call kernel_init
    jmp $