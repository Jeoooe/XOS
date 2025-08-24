#ifndef XOS_ARENA_H
#define XOS_ARENA_H

#include <xos/types.h>
#include <xos/list.h>

#define DESC_COUNT 7    //描述符数量

typedef list_node_t block_t;    //内存块

//内存描述符
typedef struct arena_descriptor_t {
    u32 total_block;    //一页内存分成多少块
    u32 block_size;     //块大小
    list_t free_list;   //空闲块的列表
} arena_descriptor_t;

//内存块
//在页的开头
typedef struct arena_t {
    arena_descriptor_t *desc;
    u32 count;  //当前剩余多少块
    u32 large;  //是否超过1024字节
    u32 magic;  //魔数，校验
} arena_t;

/// @brief 申请小块内存
/// @param size 字节数
/// @return 地址的指针
void *kmalloc(size_t size);

/// @brief 释放kmalloc得到的内存空间
/// @param ptr 对应内存的指针
void kfree(void *ptr);

#endif