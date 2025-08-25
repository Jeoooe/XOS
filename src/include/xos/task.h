#ifndef XOS_TASK_H
#define XOS_TASK_H

#include <xos/types.h>
#include <xos/bitmap.h>
#include <xos/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

typedef void target_t();

typedef enum task_state_t {
    TASK_INIT,          //初始化
    TASK_RUNNING,       //执行
    TASK_READY,         //就绪
    TASK_BLOCKED,       //阻塞
    TASK_SLEEPING,      //睡眠
    TASK_WAITING,       //等待
    TASK_DIED,          //死亡
} task_state_t;

typedef struct task_t {
    u32 *stack;                     //内核栈
    list_node_t node;               //阻塞节点
    task_state_t state;             //状态
    u32 priority;                   //优先级
    u32 ticks;                      //剩余时间片
    u32 jiffies;                    //上次执行时的全局时间片
    char name[TASK_NAME_LEN];       //名字
    u32 uid;                        //用户ID
    pid_t pid;                      //进程 id
    pid_t ppid;                     //父进程 id
    u32 pde;                        //页目录物理地址
    struct bitmap_t *vmap;          //虚拟内存的位图
    u32 brk;                        //进程堆内存最高地址
    int status;                     //进程特殊状态
    pid_t waitpid;                  //等待的进程
    u32 magic;                      //内核魔数
} task_t;

typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

//中断帧
typedef struct intr_frame_t {
    u32 vector;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp_dummy;

    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;

    u32 vector0;
    
    u32 error;

    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;
} intr_frame_t;


task_t *running_task();
void schedule();

pid_t task_fork();
void task_exit(int status);
pid_t task_waitpid(pid_t pid, int32 *status);

/// @brief 进入用户模式,调用函数的地方不能有局部变量
/// @param target 要进入的任务
void task_to_user_mode(target_t target);

void task_yield();
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(u32 ms);
void task_wakeup();

pid_t sys_getpid();
pid_t sys_getppid();

#endif