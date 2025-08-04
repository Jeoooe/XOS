#ifndef XOS_MUTEX_H
#define XOS_MUTEX_H

#include <xos/types.h>
#include <xos/list.h>

typedef struct mutex_t {
    bool value;         //信号量
    list_t waiters;     //等待队列
} mutex_t;

//初始化互斥量
void mutex_init(mutex_t *mutex);
//尝试持有互斥量    
void mutex_lock(mutex_t *mutex); 
//释放互斥量   
void mutex_unlock(mutex_t *mutex);  

typedef struct spinlock_t {
    struct task_t *holder;
    mutex_t mutex;
    u32 repeat;         //重入次数
} spinlock_t;

//锁初始化
void spin_init(spinlock_t *lock);
//加锁
void spin_lock(spinlock_t *lock);
//解锁
void spin_unlock(spinlock_t *lock);

#endif