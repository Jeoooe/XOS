#include <xos/debug.h>
#include <xos/types.h>
#include <xos/xos.h>
#include <xos/assert.h>
#include <xos/memory.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID      1
#define ZONE_RESERVED   2

#define IDX(addr) ((u32)addr >> 12)

typedef struct ards_t {
    u64 base;
    u64 size;
    u32 type;
} _packed ards_t;

u32 memory_base = 0;
u32 memory_size = 0;
u32 total_pages = 0;
u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr) {
    u32 count;
    ards_t *ptr;

    if (magic == XOS_MAGIC) {
        count = *(u32 *) addr;
        ptr = (ards_t *)(addr + 4);
        for (size_t i = 0;i < count; ++i, ++ptr) {
            LOGK("Memory base %p size %p type %d\n", 
                (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } else {
        panic("Memory init magic unknown %p\n", magic);
    }

    LOGK("ARDS Count %d\n", count);
    LOGK("Memory base: 0x%p\n", (u32)memory_base);
    LOGK("Memory size: 0x%p\n", (u32)memory_size);

    assert(memory_base == MEMORY_BASE);
    assert((memory_size & 0xfff) == 0);

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    LOGK("Total pages: %d\nFree pages: %d\n", total_pages, free_pages);
}