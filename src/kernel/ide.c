#include <xos/ide.h>
#include <xos/string.h>
#include <xos/stdio.h>
#include <xos/debug.h>
#include <xos/memory.h>
#include <xos/assert.h>
#include <xos/io.h>
#include <xos/interrupt.h>

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_CHS_SECTOR 0x0003 // CHS 扇区位置
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_CHS_CYL 0x0004    // CHS 柱面低字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_CHS_CYH 0x0005    // CHS 柱面高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令
#define IDE_CMD_READ 0x20       // 读命令
#define IDE_CMD_WRITE 0x30      // 写命令
#define IDE_CMD_IDENTIFY 0xEC   // 识别命令

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA
#define IDE_SEL_MASK 0b10110000   // CHS 模式 MASK

#define IDE_INTERFACE_UNKNOWN 0
#define IDE_INTERFACE_ATA 1
#define IDE_INTERFACE_ATAPI 2

ide_ctrl_t controllers[IDE_CTRL_NR];


//硬盘中断处理函数
void ide_handler(int vector) {
    send_eoi(vector);

    // vector = 0x2e 或 0x2f
    // 即 0x20 + IRQ_HARDDISK ( + 1)
    ide_ctrl_t *ctrl = &controllers[vector - 0x20 - IRQ_HARDDISK];

    u8 state = inb(ctrl->iobase + IDE_STATUS);
    LOGK("harddisk interrupt vector %d state 0x%x\n", vector, state);
    if (ctrl->waiter) {
        task_unblock(ctrl->waiter);
        ctrl->waiter = NULL;
    }
}


static u32 ide_error(ide_ctrl_t *ctrl) {
    u8 error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        LOGK("bad block\n");
    if (error & IDE_ER_UNC)
        LOGK("uncorrectable data\n");
    if (error & IDE_ER_MC)
        LOGK("media change\n");
    if (error & IDE_ER_IDNF)
        LOGK("id not found\n");
    if (error & IDE_ER_MCR)
        LOGK("media change requested\n");
    if (error & IDE_ER_ABRT)
        LOGK("abort\n");
    if (error & IDE_ER_TK0NF)
        LOGK("track 0 not found\n");
    if (error & IDE_ER_AMNF)
        LOGK("address mark not found\n");
}

//选择磁盘
static u32 ide_select_drive(ide_disk_t *disk) {
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

//等待磁盘控制器的状态
static u32 ide_busy_wait(ide_ctrl_t *ctrl, u8 mask) {
    while (true) {
        u8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        
        if (state & IDE_SR_ERR) {       //出现错误
            ide_error(ctrl);
        }

        if (state & IDE_SR_BSY) {       //驱动器忙
            continue;
        }

        if ((state & mask) == mask) {   //等待状态完成
            return 0;    
        }
    }
}

//选择扇区
static void ide_select_sector(ide_disk_t *disk, u32 lba, u8 count) {
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    //扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);


    //LBA低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    //LBA中字节
    outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    //LBA高字节
    outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    //LBA最高四位, 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf | disk->selector));
    disk->ctrl->active = disk;
}

//读一个扇区到buf
static void ide_pio_read_sector(ide_disk_t *disk, u16 *buf) {
    for (size_t i = 0;i < (SECTOR_SIZE / 2); i++) {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

//用buf写一个扇区
static void ide_pio_write_sector(ide_disk_t *disk, u16 *buf) {
    for (size_t i = 0;i < (SECTOR_SIZE / 2); i++) {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

/// @brief PIO方式读磁盘, 异步或同步
/// @param disk 磁盘
/// @param buf 输出区域
/// @param count 扇区数量
/// @param lba 起始扇区
/// @return 0 正常
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba) {
    assert(count > 0);
    assert(!get_interrupt_state()); //不可中断

    ide_ctrl_t *ctrl = disk->ctrl;
    
    lock_acquire(&ctrl->lock);

    //选择磁盘
    ide_select_drive(disk);
    //等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);
    //选择扇区
    ide_select_sector(disk, lba, count);
    
    //发送读命令
    outb(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0;i < count; i++) {
        task_t *task = running_task();
        //ide init时不能使用异步, 此时task->state不是running
        if (task->state == TASK_RUNNING) {
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *)offset);
    }
    lock_release(&ctrl->lock);
    return 0;
}

/// @brief PIO方式写磁盘, 异步或同步
/// @param disk 磁盘
/// @param buf 输入数组
/// @param count 扇区数量
/// @param lba 起始扇区
/// @return 0 正常
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba) {
    assert(count > 0);
    assert(!get_interrupt_state()); //不可中断

    ide_ctrl_t *ctrl = disk->ctrl;

    LOGK("write lba 0x%x\n", lba);
    
    lock_acquire(&ctrl->lock);

    //选择磁盘
    ide_select_drive(disk);
    //等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);
    //选择扇区
    ide_select_sector(disk, lba, count);
    
    //发送写命令
    outb(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0;i < count; i++) {
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (u16 *)offset);

        task_t *task = running_task();
        //ide init时不能使用异步, 此时task->state不是running
        if (task->state == TASK_RUNNING) {
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_NULL);
    }
    lock_release(&ctrl->lock);
    return 0;
}

static void ide_ctrl_init() {
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++) {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;

        //从通道
        if (cidx) {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }
        
        for (size_t disk_i = 0; disk_i < IDE_DISK_NR; disk_i++) {
            ide_disk_t *disk = &ctrl->disks[disk_i];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + disk_i);
            disk->ctrl = ctrl;
            //从盘
            if (disk_i) {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
        }
    }
}

void ide_init() {
    LOGK("ide init...\n");
    ide_ctrl_init();

    set_interupt_handler(IRQ_HARDDISK, ide_handler);
    set_interupt_handler(IRQ_HARDDISK_2, ide_handler);
    set_interupt_mask(IRQ_HARDDISK, true);
    set_interupt_mask(IRQ_HARDDISK_2, true);
    set_interupt_mask(IRQ_CASCADE, true);
}