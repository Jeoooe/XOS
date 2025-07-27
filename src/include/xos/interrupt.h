#ifndef XOS_INTERRUPT_H
#define XOS_INTERRUPT_H

#include <xos/types.h>

#define IDT_SIZE 256

//中断向量位
#define IRQ_CLOCK           0
#define IRQ_KEYBOARD        1
#define IRQ_CASCADE         2   //从片控制器
#define IRQ_SERIAL_2        3   //串口2
#define IRQ_SERIAL_1        4
#define IRQ_PARALLEL_2      5   //并口
#define IRQ_FLOPPY          6   //软盘控制器
#define IRQ_PARALLEL_1      7
#define IRQ_RTC             8
#define IRQ_REDIRECT        9   //重定向IRQ2
#define IRQ_MOUSE           12
#define IRQ_MATH            13  //协处理器 x87
#define IRQ_HARDDISK        14
#define IRQ_HARDDIST_2      15

#define IRQ_MASTER_NR       0x20  //主片向量起始
#define IRQ_SLAVE_NR        0x28  //从片向量起始

/*
    中断描述符
    8字节
*/
typedef struct gate_t {
    u16 offset0;
    u16 selector;   //代码段选择子
    u8 zero;    //无用
    // u8 type: 4; //门类型
    // u8 segment: 1;
    // u8 DPL: 2;  
    // u8 present: 1;
    u8 type_attr; //[Present, DPL, Segment, Type]
    u16 offset1;
} _packed gate_t;


//中断处理函数
typedef void *handler_t;

void send_eoi(int vector);

//设置中断处理函数
void set_interupt_handler(u32 irq, handler_t handler);
//开启和关闭中断
void set_interupt_mask(u32 irq, bool enable);


bool interrupt_disable();       //清除IF并获取清除前的值
bool get_interrupt_state();     //获取IF位
void set_interrupt_state(bool state);     //设置IF位

#endif