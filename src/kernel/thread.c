#include <xos/interrupt.h>
#include <xos/syscall.h>
#include <xos/debug.h>
#include <xos/printk.h>

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

extern u32 keyboard_read(char *buf, u32 count);

void init_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    char ch;

    while (true) {
        // LOGK("init task... %d\n", counter++);
        // sleep(500);
        bool intr = interrupt_disable();
        keyboard_read(&ch, 1);
        printk("%c", ch);

        set_interrupt_state(intr);
    }
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        // LOGK("test task... %d\n", counter++);
        sleep(799);
    }
}