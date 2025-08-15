#ifndef XOS_CONSOLE_H
#define XOS_CONSOLE_H

#include <xos/types.h>

void console_init();
void console_clear();
int32 console_write(char *buf, u32 count);

#endif