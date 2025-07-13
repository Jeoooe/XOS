#include <xos/interrupt.h>
#include <xos/types.h>
#include <xos/global.h>
#include <xos/printk.h>
#include <xos/debug.h>
#include <xos/stdlib.h>
#include <xos/io.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ENTRY_SIZE 0x30
//控制端口
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1
#define PIC_EOI 0x20



gate_t idt[IDT_SIZE];   //中断描述符表
pointer_t idt_ptr;      //中断描述符指针

handler_t handler_table[IDT_SIZE];
extern handler_t handler_entry_table[ENTRY_SIZE];

u32 counter = 0;

void send_eoi(int vector) {
    if (vector >= 0x20 && vector < 0x28) {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

void default_handler(int vector) {
    send_eoi(vector);
    LOGK("[%d] default interrupt %d", vector, counter);
    ++counter;
}

void exception_handler(int vector) {

    printk("[%d] exception interrupt", vector);

    hang();
}

void pic_init() {
    outb(PIC_M_CTRL, 0b00010001);
    outb(PIC_M_DATA, 0x20);
    outb(PIC_M_DATA, 0b00000100);
    outb(PIC_M_DATA, 0b00000001);

    outb(PIC_S_CTRL, 0b00010001);
    outb(PIC_S_DATA, 0x28);
    outb(PIC_S_DATA, 2);
    outb(PIC_S_DATA, 0b00000001);

    outb(PIC_M_DATA, 0b11111110);
    outb(PIC_S_DATA, 0b11111111);
}

void idt_init() {
    for (size_t i = 0; i < ENTRY_SIZE; ++i) {
        gate_t *gate = &idt[i];
        handler_t handler = handler_entry_table[i];
        gate->offset0 = (u32)handler & 0xffff;
        gate->offset1 = ((u32)handler >> 16) & 0xffff;
        // gate->type = 0b1110;
        // gate->DPL = 0;  //内核
        // gate->segment = 0;
        // gate->reserved = 0;
        // gate->present = 1;
        gate->selector = 1 << 3;
        gate->type_attr = 0b10001110;
        gate->zero = 0;
    }

    for (size_t i = 0;i < 0x20; ++i) {
        handler_table[i] = exception_handler;
    }

    for (size_t i = 0x20;i < ENTRY_SIZE; ++i) {
        handler_table[i] = default_handler;
    }

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;
    asm volatile("lidt idt_ptr\n");
}

void interrupt_init() {
    pic_init();
    idt_init();
}