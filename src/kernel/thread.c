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


void test_recursion() {
    char tmp[0x400];
    test_recursion();
}

static void user_init_thread() {
    u32 counter = 0;
    char ch;
    while (true) {
        printf("task is in user mode %d\n", counter++);
        BMB;
        test_recursion();
        sleep(1000);
    }
}

void init_thread() {
    // set_interrupt_state(true);
    char temp[100]; //给栈顶留出足够的空间
    task_to_user_mode(user_init_thread);
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        LOGK("test task %d...\n", counter++);
        BMB;
        sleep(2000);
    }
}