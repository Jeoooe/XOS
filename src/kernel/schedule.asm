[bits 32]

section .text

global task_switch
task_switch:
    push ebp
    mov ebp, esp

    ; 保存栈帧
    push ebx
    push esi
    push edi

    mov eax, esp
    and eax, 0xfffff000; 当前栈顶

    mov [eax], esp; 保存当前栈顶

    mov eax, [ebp + 8]
    mov esp, [eax]  ;切换到任务指针指向的栈顶

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
