#ifndef XOS_GLOBAL_H
#define XOS_GLOBAL_H

#include <xos/types.h>

#define GDT_SIZE 128

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX  3

#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)

#define USER_CODE_SELECTOR (USER_CODE_IDX << 3 | 0b11)
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3 | 0b11)



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

//任务状态段描述符
typedef struct tss_t {
    u32 backlink;   //前一个任务的链接(指向任务状态段的段选择子)
    u32 esp0;
    u32 ss0;
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 flags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldtr;
    u16 trace: 1;
    u16 reversed: 15;
    u16 iobase;
    u32 ssp;
} _packed tss_t;

void gdt_init();

#endif