#include <xos/debug.h>
#include <xos/interrupt.h>
#include <xos/types.h>
#include <xos/syscall.h>
#include <xos/assert.h>
#include <xos/task.h>
#include <xos/console.h>
#include <xos/memory.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define SYSCALL_GATE 64


handler_t syscall_table[SYSCALL_GATE];

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_GATE) {
        panic("syscall nr error");
    }
}

static void sys_default() {
    panic("syscall not implemented");
}

task_t *task = NULL;

static u32 sys_test() {
    // LOGK("syscall test \n");

    char *ptr;

    // BMB;
    // *ptr = (char *) 0x1600000;
    // ptr[3] = 'T';
    // BMB;

    BMB;
    link_page(0x1600000);
    BMB;

    ptr = (char *) 0x1600000;
    ptr[3] = 'T';
    BMB;

    unlink_page(0x1600000);

    BMB;
    return 255;
}


int32 sys_write(fd_t fd, char *buf, u32 len) {
    if (fd == stdout || fd == stderr) {
        return console_write(buf, len);
    }
    //TODO
    panic("write!!");
    return 0;
}

void syscall_init() {
    for (size_t i = 0;i < SYSCALL_GATE; i++) {
        syscall_table[i] = sys_default;
    }
    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_WRITE] = sys_write;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;
}