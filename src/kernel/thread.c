#include <xos/interrupt.h>
#include <xos/syscall.h>
#include <xos/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void idle_thread() {
    set_interrupt_state(true);
    // u32 counter = 0;
    while (true) {
        // LOGK("idle task... %d\n", counter++);
        asm volatile(
            "sti\n" //打开中断
            "hlt\n" //关闭CPU，等待外中断
        );
        yield();
    }
}

#include <xos/mutex.h>

spinlock_t lock;


void init_thread() {
    spin_init(&lock);
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        spin_lock(&lock);
        LOGK("init task... %d\n", counter++);
        spin_unlock(&lock);
        // sleep(500);
    }
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        spin_lock(&lock);
        LOGK("test task... %d\n", counter++);
        spin_unlock(&lock);
        // sleep(799);
    }
}