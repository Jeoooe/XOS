#include <xos/assert.h>
#include <xos/printk.h>
#include <xos/types.h>
#include <xos/stdarg.h>
#include <xos/stdio.h>

static u8 *buf[1024];

static void spin(char *name) {
    printk("spinning in %s \n", name);
    while (true)
        ;
}

void assertion_failure(char *exp, char *file, char *base, int line) {
    printk("\n--> assert(%s) fail\n"
    "--> file: %s\n"
    "--> base_file: %s\n"
    "--> line: %d \n",
    exp, file, base, line);

    spin("assertion_failure");
    asm volatile("ud2");
}

void panic(const char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    printk("---panic--- \n%s\n", buf);
    spin("panic");

    asm volatile("ud2");
}