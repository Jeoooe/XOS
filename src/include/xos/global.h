#ifndef XOS_GLOBAL_H
#define XOS_GLOBAL_H

#include <xos/types.h>

#define GDT_SIZE 128

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)

//描述符
typedef struct descriptor_t {
    u16 limit_low;      //段界限0-15
    u32 base_low: 24;
    u8 type: 4;
    u8 segment: 1;      //1 = 代码段，数据段
    u8 DPL: 2;          //权限等级
    u8 present: 1;      //1在内存
    u8 limit_high: 4;
    u8 available: 1;    //无用
    u8 long_mode: 1;    //64位
    u8 big: 1;          //32位
    u8 granularity: 1;  //粒度=4KB
    u8 base_high;       //24-31
} _packed descriptor_t;

//段选择子
typedef struct selector_t {
    u8 RPL: 2;
    u8 TI: 1;
    u16 index: 13;
} selector_t;

//描述符表指针
typedef struct pointer_t
{
    u16 limit;
    u32 base;
} _packed pointer_t;

void gdt_init();

#endif