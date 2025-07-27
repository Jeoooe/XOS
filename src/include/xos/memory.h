#ifndef XOS_MEMORY_H
#define XOS_MEMORY_H

#include <xos/types.h>

#define PAGE_SIZE       0x1000
#define MEMORY_BASE     0x100000

//内核页目录
#define KERNEL_PAGE_DIR     0x1000

static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000
};

typedef struct page_entry_t {
    u8 present: 1;
    u8 write: 1;
    u8 user: 1;
    u8 pwt: 1;          //page write through 直写模式
    u8 pcd: 1;          //page cache disable禁止页缓冲
    u8 accessed: 1;
    u8 dirty: 1;        //页缓冲是否被写过
    u8 pat: 1;          //page attribute table 页大小 4k / 4M
    u8 global: 1;
    u8 ignored: 3;
    u32 index: 20;
} _packed page_entry_t;

u32 get_cr3();
void set_cr3(u32 pde);

//分配内核页
u32 alloc_kpage(u32 count);
//释放连续内核页
void free_kpages(u32 addr, u32 count);

#endif