#ifndef XOS_IDE_H
#define XOS_IDE_H

#include <xos/types.h>
#include <xos/mutex.h>
#include <xos/task.h>

#define SECTOR_SIZE 512     // 扇区大小

#define IDE_CTRL_NR 2   //控制器数量
#define IDE_DISK_NR 2   //控制器可挂载磁盘数量

//IDE 磁盘
typedef struct ide_disk_t {
    char name[8];                   //磁盘名字
    struct ide_ctrl_t *ctrl;        //控制器指针
    u8 selector;                    //磁盘选择
    bool master;                    //是否是主盘
} ide_disk_t;

//IDE 控制器
typedef struct ide_ctrl_t {
    char name[8];
    lock_t lock;
    u16 iobase;                     //IO寄存器基地址
    ide_disk_t disks[IDE_DISK_NR];  //磁盘
    ide_disk_t *active;             //当前磁盘
    task_t *waiter;
} ide_ctrl_t;

/// @brief 从磁盘中读取数据
/// @param disk 磁盘
/// @param buf 目标内存空间
/// @param count 扇区数量
/// @param lba 起始扇区
/// @return 
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif