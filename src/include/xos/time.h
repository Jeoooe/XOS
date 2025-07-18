#ifndef XOS_TIME_H
#define XOS_TIME_H

#include <xos/types.h>

typedef struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;    // [0, 31]
    int tm_mon;
    int tm_year;
    int tm_wday;    // [0,6]
    int tm_yday;
    int tm_isdst;
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time);

#endif