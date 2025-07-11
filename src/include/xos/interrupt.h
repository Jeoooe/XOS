#ifndef XOS_INTERRUPT_H
#define XOS_INTERRUPT_H

#include <xos/types.h>

#define IDT_SIZE 256

/*
    中断描述符
    8字节
*/
typedef struct gate_t {
    u16 offset0;
    u16 selector;   //代码段选择子
    u8 reserved;    //无用
    u8 type: 4; //门类型
    u8 segment: 1;
    u8 DPL: 2;  
    u8 present: 1;
    u16 offset1;
} _packed gate_t;


//中断处理函数
typedef void *handler_t;

void interrupt_init();

#endif