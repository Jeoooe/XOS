#include <xos/interrupt.h>
#include <xos/types.h>
#include <xos/global.h>
#include <xos/printk.h>
#include <xos/debug.h>

#define ENTRY_SIZE 0x20

gate_t idt[IDT_SIZE];   //中断描述符表
pointer_t idt_ptr;      //中断描述符指针

handler_t handler_table[IDT_SIZE];
extern handler_t handler_entry_table[ENTRY_SIZE];

void exception_handler(int vector) {

    printk("Exception: %d", vector);

    while (true) 
        ;
}

void interrupt_init() {
    for (size_t i = 0; i < ENTRY_SIZE; ++i) {
        gate_t *gate = &idt[i];
        handler_t handler = handler_entry_table[i];
        gate->offset0 = (u32)handler & 0xffff;
        gate->offset1 = ((u32)handler >> 16) & 0xffff;
        gate->DPL = 0;  //内核
        gate->segment = 0;
        gate->type = 0b1110;
        gate->reserved = 0;
        gate->selector = 1 << 3;
        gate->present = 1;
    }

    for (size_t i = 0;i < ENTRY_SIZE; ++i) {
        handler_table[i] = exception_handler;
    }

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;
    BMB;
    asm volatile("lidt idt_ptr\n");
}