#ifndef XOS_DEBUG_H
#define XOS_DEBUG_H

void debugk(char *file, int line, const char *fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx")
#define DEBUGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#endif