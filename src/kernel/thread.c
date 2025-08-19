#include <xos/interrupt.h>
#include <xos/syscall.h>
#include <xos/debug.h>
#include <xos/printk.h>
#include <xos/task.h>
#include <xos/stdio.h>
#include <xos/arena.h>

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


static void real_init_thread() {
    u32 counter = 0;
    char ch;
    while (true) {
        BMB;
        sleep(100);
        // printf("task is in user mode %d\n", counter ++);
    }
}

void init_thread() {
    // set_interrupt_state(true);
    char temp[100]; //给栈顶留出足够的空间
    task_to_user_mode(real_init_thread);
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        void *ptr = kmalloc(1200);
        LOGK("kmalloc 0x%p...\n", ptr);
        kfree(ptr);

        ptr = kmalloc(1024);
        LOGK("kmalloc 0x%p...\n", ptr);
        kfree(ptr);

        ptr = kmalloc(54);
        LOGK("kmalloc 0x%p...\n", ptr);
        kfree(ptr);

        sleep(2000);
    }
}