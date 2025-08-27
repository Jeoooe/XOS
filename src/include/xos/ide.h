#ifndef XOS_IDE_H
#define XOS_IDE_H

#include <xos/types.h>
#include <xos/mutex.h>
#include <xos/task.h>

#define SECTOR_SIZE 512     // 扇区大小

#define IDE_CTRL_NR 2   //控制器数量
#define IDE_DISK_NR 2   //控制器可挂载磁盘数量
#define IDE_PART_NR 4   //磁盘分区数量, 仅主分区

//分区表, 主引导扇区前16字节
typedef struct part_entry_t {
    u8 bootable;            //引导标志
    u8 start_head;          //起始磁头
    u8 start_sector: 6;     //起始扇区
    u16 start_cylinder: 10; //起始柱面号
    u8 system;              //分区字节类型
    u8 end_head;            //分区结束磁头号
    u8 end_sector: 6;       //分区结束扇区
    u16 end_cylinder: 10;   //分区结束柱面
    u32 start;              //分区起始物理扇区号 LBA
    u32 count;              //分区占用扇区数
} _packed part_entry_t;

//主引导扇区内容
typedef struct boot_sector_t {
    u8 code[446];
    part_entry_t entry[4];
    u16 signature;          //55aa
} _packed boot_sector_t;

//分区信息
typedef struct ide_part_t {
    char name[8];
    struct ide_disk_t *disk;        //磁盘指针
    u32 system;                     //分区类型
    u32 start;
    u32 count;
} ide_part_t;

//IDE 磁盘
typedef struct ide_disk_t {
    char name[8];                   //磁盘名字
    struct ide_ctrl_t *ctrl;        //控制器指针
    u8 selector;                    //磁盘选择
    bool master;                    //是否是主盘
    u32 total_lba;                  //可用扇区数量
    u32 cylinders;                  //柱面数
    u32 heads;                      //磁头数
    u32 sectors;                    //扇区数
    ide_part_t parts[IDE_PART_NR];  //硬盘分区
} ide_disk_t;       


//IDE 控制器
typedef struct ide_ctrl_t {
    char name[8];
    lock_t lock;
    u16 iobase;                     //IO寄存器基地址
    ide_disk_t disks[IDE_DISK_NR];  //磁盘
    ide_disk_t *active;             //当前磁盘
    u8 control;                     //控制字节
    task_t *waiter;                 //等待任务
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