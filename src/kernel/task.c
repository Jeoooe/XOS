#include <xos/task.h>
#include <xos/printk.h>
#include <xos/debug.h>
#include <xos/memory.h>
#include <xos/bitmap.h>
#include <xos/assert.h>
#include <xos/interrupt.h>
#include <xos/xos.h>
#include <xos/string.h>
#include <xos/syscall.h>

extern bitmap_t kernel_map;
extern void task_switch(task_t *next);

#define NR_TASKS 64
static task_t *task_table[NR_TASKS];

//获取空闲的任务
static task_t *get_free_task() {
    for (size_t i = 0;i < NR_TASKS; i++) {
        if (task_table[i] == NULL) {
            task_table[i] = (task_t *)alloc_kpage(1);
            return task_table[i];
        }
    }
    panic("No more tasks");
}

static task_t *task_search(task_state_t state) {
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *current = running_task();

    for (size_t i = 0;i < NR_TASKS; i++) {
        task_t *ptr = task_table[i];
        if (ptr == NULL) 
            continue;
        

        if (ptr->state != state)
            continue;
        if (current == ptr) 
            continue;
        
        //执行时间最长的，执行里面最晚的
        if (task == NULL || task->ticks < ptr->ticks || ptr->jiffies < task->jiffies)
            task = ptr;
    }

    return task;
}

void task_yield() {
    schedule();
}

task_t *running_task() {
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n"
    );
}

void schedule() {
    assert(!get_interrupt_state()); //不可中断

    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);

    assert(next != NULL);
    assert(next->magic == XOS_MAGIC);

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }

    next->state = TASK_RUNNING;
    if (next == current) {
        return;
    }
    task_switch(next);
}
 
static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid) {
    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);

    u32 stack = (u32)task + PAGE_SIZE;

    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    strcpy((char *)task->name, name);

    task->stack = (u32 *)stack;
    task->priority = priority;
    task->ticks = priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_map;
    task->pde = KERNEL_PAGE_DIR;
    task->magic = XOS_MAGIC;

    return task;
}

static void task_setup() {
    task_t *task = running_task();
    task->magic = XOS_MAGIC;
    task->ticks = 1;

    memset(task_table, 0, sizeof(task_table));
}

u32 tha() {
    set_interrupt_state(true);
    while (true) {
        printk("A");
        yield();
    }
}
u32 thb() {
    set_interrupt_state(true);
    while (true) {
        printk("B");
        yield();
    }
}
u32 thc() {
    set_interrupt_state(true);
    while (true) {
        printk("C");
        yield();
    }
}

void task_init() {
    task_setup();
    task_create(tha, "a", 5, KERNEL_USER);
    task_create(thb, "b", 5, KERNEL_USER);
    task_create(thc, "c", 5, KERNEL_USER);
}