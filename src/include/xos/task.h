#ifndef XOS_TASK_H
#define XOS_TASK_H

#include <xos/types.h>

typedef u32 target_t();

typedef struct task_t {
    u32 *stack;
} task_t;

typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

void task_init();

#endif