#ifndef XOS_DEVICE_H
#define XOS_DEVICE_H

#include <xos/types.h>

#define DEVICE_NAME_LEN 16

enum device_type_t {
    DEV_NULL,
    DEV_CHAR,   //字符设备 char单位读写
    DEV_BLOCK,  //块设备
};

enum device_subtype_t {
    DEV_CONSOLE = 1,
    DEV_KEYBOARD,
};

typedef struct device_t {
    char name[DEVICE_NAME_LEN];         //设备名
    int type;                           //设备类型
    int subtype;                        //设备子类型
    dev_t dev;                          //设备号
    dev_t parent;                       //父设备号
    void *ptr;                          //设备指针
    //设备控制
    int (*ioctl)(void *dev, int cmd, void *args, int flags);
    //读设备
    int (*read)(void *dev, void *buf, size_t count, idx_t idx, int flags);
    //写设备
    int (*write)(void *dev, void *buf, size_t count, idx_t idx, int flags);
} device_t;

/// @brief 安装设备
/// @param type 设备类型
/// @param substype 子类型
/// @param ptr 设备指针
/// @param name 设备名
/// @param parent 父设备号
/// @param ioctl 设备控制函数
/// @param read 读函数
/// @param write 写函数
/// @return 设备号
dev_t device_install(
    int type, int substype, void *ptr, char *name, dev_t parent,
    void *ioctl, void *read, void *write
);


/// @brief 根据子类型查找设备
/// @param type 子类型
/// @param idx 找到第几个
/// @return 设备指针
device_t *device_find(int type, idx_t idx);

//设备号查找设备
device_t *device_get(dev_t dev);

//控制
int device_ioctl(dev_t dev, int cmd, void *args, int flags);

//读设备
int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

//写设备
int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags);



#endif