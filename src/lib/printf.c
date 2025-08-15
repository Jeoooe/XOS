#include <xos/stdio.h>
#include <xos/stdarg.h>
#include <xos/syscall.h>

static char buf[1024];  //最多可以打印1024个字符

int printf(const char *fmt, ...) {
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    write(stdout, buf, i);

    return i;
}