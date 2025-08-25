#ifndef XOS_MEMORY_H
#define XOS_MEMORY_H

/*
 * PDE page directory   entry   页目录
 * PTE page table       entry   页表
 */

#include <xos/types.h>

#define PAGE_SIZE       0x1000
#define MEMORY_BASE     0x100000

//内核占用内存的大小 8M
#define KERNEL_MEMORY_SIZE  0x800000

//用户栈顶的地址 128M
#define USER_STACK_TOP 0x8000000

//用户栈最大
#define USER_STACK_SIZE 0x200000
//用户栈底地址
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

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

//cr2 寄存器
u32 get_cr2();

u32 get_cr3();


/// @brief 设置cr3寄存器
/// @param pde 页目录地址
void set_cr3(u32 pde);

//分配内核页
u32 alloc_kpage(u32 count);
//释放连续内核页
void free_kpage(u32 addr, u32 count);

// vaddr映射物理内存
void link_page(u32 vaddr);
// 去掉vaddr映射
void unlink_page(u32 vaddr);

//拷贝当前页目录
page_entry_t *copy_pde();

//系统调用 brk : 修改用户的堆内存最大值
int32 sys_brk(void *addr);

#endif