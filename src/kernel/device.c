#include <xos/device.h>
#include <xos/string.h>
#include <xos/assert.h>
#include <xos/debug.h>
#include <xos/arena.h>


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

        list_init(&device->request_list);
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


static void do_request(request_t *req) {
    switch (req->type) {
        case REQ_READ:
            device_read(req->dev, req->buf, req->count, req->idx, req->flags);
            break;
        case REQ_WRITE:
            device_write(req->dev, req->buf, req->count, req->idx, req->flags);
            break;
        default:
            panic("req type %d unknown", req->type);
            break;
    }
}

void device_request(dev_t dev, void *buf, 
    u8 count, idx_t idx, int flags, u32 type) {
    device_t *device = device_get(dev);
    assert(device->type == DEV_BLOCK);
    idx_t offset = idx + device_ioctl(device->dev, DEV_CMD_SECTOR_START, 0, 0);

    //有父设备，则是分区
    //要操作磁盘
    if (device->parent) {
        device = device_get(device->parent);
    }

    request_t *req = kmalloc(sizeof(request_t));
    req->dev = dev;
    req->buf = buf;
    req->count = count;
    req->idx = offset;
    req->flags = flags;
    req->type = type;
    req->task = NULL;

    bool empty = list_empty(&device->request_list);

    list_push(&device->request_list, &req->node);

    //请求列表不为空
    if (!empty) {
        req->task = running_task();
        task_block(req->task, NULL, TASK_BLOCKED);
    }

    do_request(req);

    list_remove(&req->node);
    kfree(req);

    if (!list_empty(&device->request_list)) {
        request_t *nextreq = element_entry(request_t, node, device->request_list.tail.prev);
        assert(nextreq->task->magic == XOS_MAGIC);
        task_unblock(nextreq->task);
    }
}