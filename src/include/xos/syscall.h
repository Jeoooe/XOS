#ifndef XOS_SYSCALL_H
#define XOS_SYSCALL_H

#include <xos/types.h>

typedef enum syscall_t {
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_WRITE = 4,
    SYS_NR_WAITPID = 7,
    SYS_NR_GETPID = 20,
    SYS_NR_BRK = 45,
    SYS_NR_GETPPID = 64,
    SYS_NR_YIELD = 158,
    SYS_NR_SLEEP = 162,
} syscall_t;

u32 test();

//创建子进程
pid_t fork();
//退出进程
void exit(int status);

/// @brief 等待进程状态
/// @param pid 等待的pid, -1为任意进程
/// @param status 进程退出的状态
/// @return 进程pid, -1则没找到对应进程
pid_t waitpid(pid_t pid, int32 *status);

void yield();
void sleep(u32 ms);

//获取进程 id
pid_t getpid();
//父获取进程 id
pid_t getppid();

int32 brk(void *addr);

int32 write(fd_t fd, char* buf, u32 len);


#endif