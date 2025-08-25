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

static void user_init_thread() {
    // u32 counter = 0;
    int status;
    while (true) {
        pid_t pid = fork();

        if (pid) {
            printf("fork after parent %d, %d, %d...\n", pid, getpid(), getppid());
            pid_t child = waitpid(pid, &status);
            printf("wait pid %d status %d %d\n", child, status, time());
        }
        else {
            printf("fork after child %d, %d, %d...\n", pid, getpid(), getppid());
            sleep(1000);
            exit(0);
        }
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
        printf("init thread %d %d %d...\n", getpid(), getppid(), counter++);
        sleep(2000);
    }
}