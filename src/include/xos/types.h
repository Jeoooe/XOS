#ifndef XOS_TYPES_H
#define XOS_TYPES_H

#include <xos/xos.h>

#define EOF -1

#define NULL ((void*) 0)

#define EOS '\0'

#ifndef __cplusplus
#define bool _Bool
#define true 1
#define false 0
#endif

#define _ofp __attribute__((optimize("omit-frame-pointer")))
#define _packed __attribute__((packed)) //定义特殊结构 gcc

typedef unsigned int size_t;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u32 time_t;

#endif