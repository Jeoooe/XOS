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
#include <xos/list.h>
#include <xos/global.h>
#include <xos/arena.h>

#define NR_TASKS 64

extern u32 volatile jiffies;
extern u32 jiffy;
extern bitmap_t kernel_map;
extern tss_t tss;

extern void task_switch(task_t *next);


static task_t *task_table[NR_TASKS];        //任务表
static list_t block_list;                   //任务默认阻塞链表
static list_t sleep_list;
static task_t *idle_task;                   //空闲进程

//获取空闲的任务
static task_t *get_free_task() {
    for (size_t i = 0;i < NR_TASKS; i++) {
        //进程为空
        if (task_table[i] == NULL) {
            task_t *task = (task_t *)alloc_kpage(1);
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            task_table[i] = task;
            return task;
        }
    }
    panic("No more tasks");
}

pid_t sys_getpid() {
    const task_t *task = running_task();
    return task->pid;
}
pid_t sys_getppid() {
    const task_t *task = running_task();
    return task->ppid;
}

//从任务表里寻找state的任务
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

    if (task == NULL && state == TASK_READY) {
        task = idle_task;
    }

    return task;
}

void task_yield() {
    schedule();
}

/// @brief 阻塞进程
/// @param task 要阻塞的进程
/// @param blist 阻塞列表, NULL则使用默认列表
/// @param state 设置进程的状态
void task_block(task_t *task, list_t *blist, task_state_t state) {
    assert(!get_interrupt_state());
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);

    if (blist == NULL) {
        blist = &block_list;
    }

    list_push(blist, &task->node);

    task->state = state;

    task_t *current = running_task();
    if (current == task) {
        schedule();
    }
}

//解除阻塞
void task_unblock(task_t *task) {
    assert(!get_interrupt_state());

    list_remove(&task->node);

    task->state = TASK_READY;
}

void task_sleep(u32 ms) {
    assert(!get_interrupt_state()); //不可中断

    u32 ticks = ms / jiffy;         //睡眠所需的时间片
    ticks = ticks > 0 ? ticks : 1;

    //记录全局时间片，在该时刻唤醒
    task_t *current = running_task();
    current->ticks = jiffies + ticks;

    //插入到睡眠列表中
    //列表维护有序，按时间点从早到晚
    list_t *list = &sleep_list;
    list_node_t *anchor = &list->tail;

    for (list_node_t *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next) {
        task_t *task = element_entry(task_t, node, ptr);

        if (task->ticks > current->ticks) {
            anchor = ptr;
            break;
        }
    }

    //没有被阻塞
    assert(current->node.next == NULL);
    assert(current->node.prev == NULL);


    list_insert_before(anchor, &current->node);

    current->state = TASK_SLEEPING;

    schedule();
}

void task_wakeup() {
    assert(!get_interrupt_state()); //不可中断

    list_t *list = &sleep_list;
    for (list_node_t *ptr = list->head.next;ptr != &list->tail;) {
        task_t *task = element_entry(task_t, node, ptr);
        if (task->ticks > jiffies) {
            break;
        }

        //由于unblock会清空node指针，因此要先递增
        ptr = ptr->next;

        task->ticks = 0;
        task_unblock(task);
    }
}

void task_activate(task_t *task) {
    assert(task->magic == XOS_MAGIC);

    if (task->pde != get_cr3()) {
        set_cr3(task->pde);
    }

    if (task->uid != KERNEL_USER) {
        tss.esp0 = (u32)task + PAGE_SIZE;
    }
}

//获取当前正在运行的任务
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

    task_activate(next);
    task_switch(next);
}
 
static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid) {
    task_t *task = get_free_task();

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
    task->brk = KERNEL_MEMORY_SIZE;
    task->magic = XOS_MAGIC;

    return task;
}

void task_to_user_mode(target_t target) {
    task_t *task = running_task();

    //创建用户进程虚拟内存位置
    task->vmap = kmalloc(sizeof(bitmap_t)); //todo kfree
    void *buf = (void *)alloc_kpage(1);     //todo free_kpage
    bitmap_init(task->vmap, buf, PAGE_SIZE, KERNEL_MEMORY_SIZE / PAGE_SIZE);

    //创建用户进程页表
    task->pde = (u32)copy_pde();
    set_cr3(task->pde);

    u32 addr = (u32)task + PAGE_SIZE;

    addr -= sizeof(intr_frame_t);
    intr_frame_t* iframe = (intr_frame_t*)(addr);

    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = XOS_MAGIC;

    iframe->eip = (u32)target;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = USER_STACK_TOP;

    asm volatile(
        "movl %0, %%esp\n"
        "jmp interrupt_exit\n"::"m"(iframe)
    );
}

extern void interrupt_exit();

//构建进程的栈
static void task_build_stack(task_t *task) {
    u32 addr = (u32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)addr;
    iframe->eax = 0;        //返回值为0

    addr -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)addr;

    frame->ebp = 0xaa55aa55;
    frame->ebx = 0xaa55aa55;
    frame->edi = 0xaa55aa55;
    frame->esi = 0xaa55aa55;

    frame->eip = interrupt_exit;

    task->stack = (u32 *)frame;
}

pid_t task_fork() {
    task_t *task = running_task();

    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);

    task_t *child = get_free_task();
    const pid_t pid = child->pid;

    memcpy(child, task, PAGE_SIZE);

    child->pid = pid;
    child->ppid = task->pid;
    child->ticks = child->priority;
    child->state = TASK_READY;

    //拷贝虚拟内存位图
    child->vmap = kmalloc(sizeof(bitmap_t));    //todo kfree
    memcpy(child->vmap, task->vmap, sizeof(bitmap_t));

    void *buf = (void *)alloc_kpage(1);         //todo free_page
    memcpy(buf, task->vmap->bits, PAGE_SIZE);
    child->vmap->bits = buf;

    child->pde = (u32)copy_pde();
    
    task_build_stack(child);

    //父进程返回子进程id
    return child->pid;
}

void task_exit(int status) {
    task_t *task = running_task();
    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);

    task->state = TASK_DIED;
    task->status = status;

    free_pde();
    free_kpage((u32)task->vmap->bits, 1);
    kfree(task->vmap);

    //将子进程的父进程赋值为自己的父进程
    for (size_t i = 0; i < NR_TASKS; i++) {
        task_t *child = task_table[i];
        if (!child) continue;
        if (child->ppid != task->pid) continue;
        child->ppid = task->ppid;
    }
    LOGK("task 0x%p exit...\n", task);

    task_t *parent = task_table[task->ppid];
    if (parent->state == TASK_WAITING && 
    (parent->waitpid == -1 || parent->waitpid == task->pid)) {
        task_unblock(parent);
    }
    schedule();
}

pid_t task_waitpid(pid_t pid, int32 *status) {
    task_t *task = running_task();
    task_t *child = NULL;

    while (true) {
        bool has_child = false;
        for (size_t i = 2;i < NR_TASKS; i++) {
            task_t *ptr = task_table[i];
            if (!ptr) continue;
            if (ptr->ppid != task->pid) continue;
            if (ptr->pid != pid && pid != -1) continue;

            //若子进程已经exit
            if (ptr->state == TASK_DIED) {
                child = ptr;
                task_table[i] = NULL;
                goto rollback;
            }

            has_child = true;
        }
        if (has_child) {
            task->waitpid = pid;
            task_block(task, NULL, TASK_WAITING);
            continue;
        }
        break;
    }

    return -1;  //没找到对应子进程


rollback:
    *status = child->status;
    const u32 ret = child->pid;
    free_kpage((u32)child, 1);
    return ret;
}

static void task_setup() {
    task_t *task = running_task();
    task->magic = XOS_MAGIC;
    task->ticks = 1;

    memset(task_table, 0, sizeof(task_table));
}

extern void idle_thread();
extern void init_thread();
extern void test_thread();

void task_init() {
    list_init(&block_list);
    list_init(&sleep_list);

    task_setup();
    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
    task_create(init_thread, "init", 5, NORMAL_USER);
    task_create(test_thread, "test", 5, NORMAL_USER);
}