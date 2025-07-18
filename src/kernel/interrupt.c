#include <xos/interrupt.h>
#include <xos/types.h>
#include <xos/global.h>
#include <xos/printk.h>
#include <xos/debug.h>
#include <xos/stdlib.h>
#include <xos/io.h>
#include <xos/task.h>
#include <xos/assert.h>

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

void send_eoi(int vector) {
    if (vector >= 0x20 && vector < 0x28) {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

void set_intterupt_handler(u32 irq, handler_t handler) {
    assert(irq >= 0 && irq < 16);
    handler_table[IRQ_MASTER_NR + irq] = handler;
}

//开启和关闭中断
void set_intterupt_mask(u32 irq, bool enable)  {
    assert(irq >= 0 && irq < 16);
    u16 port;
    if (irq < 8) {
        port = PIC_M_DATA;
    } else {
        port = PIC_S_DATA;
        irq -= 8;
    }
    // 位=1是关闭
    if (enable) {
        outb(port, inb(port) & ~(1 << irq));
    } else {
        outb(port, inb(port) | (1 << irq));
    }
}

u32 counter = 0;

void default_handler(int vector) {
    send_eoi(vector);
    DEBUGK("[%x] default interrupt called %d", vector, counter);
    ++counter;
}

void exception_handler(
    int vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags) {

    printk("\n ---EXCEPTION---");
    printk("\n VECTOR: %d", vector);
    printk("\n  ERROR: %d", error);
    printk("\n EFLAGS: %d", eflags);
    printk("\n     CS: %d", cs);
    printk("\n    EIP: %d", eip);
    printk("\n    ESP: %d", esp);

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

    outb(PIC_M_DATA, 0b11111111);   //关闭所有中断
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