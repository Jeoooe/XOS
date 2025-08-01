#ifndef XOS_STDIO_H
#define XOS_STDIO_H

#include <xos/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif