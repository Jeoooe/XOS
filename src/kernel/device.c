#include <xos/device.h>
#include <xos/string.h>
#include <xos/assert.h>
#include <xos/debug.h>

#define DEVICE_NR 64

//设备数组,0置空
static device_t devices[DEVICE_NR];

static device_t *get_null_device() {
    for (size_t i = 1;i < DEVICE_NR;i++) {
        device_t *device = &devices[i];
        if (device->type == DEV_NULL) {
            return device;
        }
    }
    panic("no more devices");
}

void device_init() {
    for (size_t i = 0; i < DEVICE_NR;i++) {
        device_t *device = &devices[i];
        strcpy(device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = NULL;
        device->read = NULL;
        device->write = NULL;
    }
}

dev_t device_install(
    int type, int substype, void *ptr, char *name, dev_t parent,
    void *ioctl, void *read, void *write
) {
    device_t *device = get_null_device();
    strncpy(device->name, name, DEVICE_NAME_LEN);
    device->type = type;
    device->subtype = substype;
    device->parent = parent;
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;
    device->ptr = ptr;
    return device->dev;
}

//根据子类型查找设备
device_t *device_find(int type, idx_t idx) {
    idx_t count = 0;
    for (size_t i = 0;i < DEVICE_NR;i++) {
        device_t *device = &devices[i];
        if (device->subtype != type) continue;
        if (count == idx) {
            return device;
        }
        count++;
    }
    return NULL;
}

//设备号查找设备
device_t *device_get(dev_t dev) {
    assert(0 < dev && dev < DEVICE_NR);
    device_t *device = &devices[dev];
    assert(device->type != DEV_NULL);
    return device;
}


int device_ioctl(dev_t dev, int cmd, void *args, int flags) {
    device_t *device = &devices[dev];
    if (device->ioctl) {
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    LOGK("ioctl of device %d not implemented\n", dev);
    return EOF;
}

int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags) {
    device_t *device = &devices[dev];
    if (device->read) {
        return device->read(device->ptr, buf, count, idx, flags);
    }
    LOGK("read of device %d not implemented\n", dev);
    return EOF;
}

int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags) {
    device_t *device = &devices[dev];
    if (device->write) {
        return device->write(device->ptr, buf, count, idx, flags);
    }
    LOGK("write of device %d not implemented\n", dev);
    return EOF;
}


