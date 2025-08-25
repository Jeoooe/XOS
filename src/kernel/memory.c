#include <xos/debug.h>
#include <xos/types.h>
#include <xos/xos.h>
#include <xos/assert.h>
#include <xos/memory.h>
#include <xos/stdlib.h>
#include <xos/string.h>
#include <xos/bitmap.h>
#include <xos/multiboot2.h>
#include <xos/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID      1
#define ZONE_RESERVED   2

//获取页索引
#define IDX(addr) ((u32)addr >> 12)         
//获取页目录索引
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)  
//获取页表索引
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff)  
//根据页获取地址
#define PAGE(idx) ((u32)idx << 12)
//断言addr是页的开始
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)
//页目录蒙版
#define PDE_MASK 0xFFC00000

#define KERNEL_MAP_BITS 0x4000

// #define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

bitmap_t kernel_map;    //内核位图

typedef struct ards_t {
    u64 base;
    u64 size;
    u32 type;
} _packed ards_t;

static u32 memory_base = 0;
static u32 memory_size = 0;
static u32 total_pages = 0;
static u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr) {
    u32 count = 0;
    //自定义的bootloader
    if (magic == XOS_MAGIC) {
        count = *(u32 *) addr;
        ards_t *ptr = (ards_t *)(addr + 4);
        for (size_t i = 0;i < count; ++i, ++ptr) {
            LOGK("Memory base %p size %p type %d\n", 
                (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } 
    //multiboot2
    else if (magic == MULTIBOOT2_MAGIC ) {
        u32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        LOGK("Annouced mbi size 0x%x\n", size);
        while (tag->type != MULTIBOOT_TAG_TYPE_END) {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
                break;
            //下一个tag,对齐8字节
            tag = (multi_tag_t*)((u32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t* mtag = (multi_tag_mmap_t*)tag;
        multi_mmap_entry_t* entry = mtag->entries;
        //遍历entries
        while ((u32)entry < (u32)tag + tag->size) {
            LOGK("Memory base 0x%p size 0x%p type %d\n", 
                (u32)entry->addr, (u32)entry->len, (u32)entry->type);
            count++;
            if (entry->type == ZONE_VALID && entry->len > memory_size) {
                memory_base = (u32)entry->addr;
                memory_size = (u32)entry->len;
            }
            entry = (multi_mmap_entry_t*)((u32)entry + mtag->entry_size);
        }
    }
    else {
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

    if (memory_size < KERNEL_MEMORY_SIZE) {
        panic("System memory is %dM too small, at least %dM needed\n",
            memory_size / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static u32 start_page = 0;
static u8 *memory_map;
static u32 memory_map_pages;

void memory_map_init() {

    // memory_map, 物理内存数组
    // 存放对应idx内存页的引用数量
    memory_map = (u8 *)memory_base;

    // 物理内存数组所需要的内存页数
    // 总共需要存放total_pages个引用
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
    LOGK("Memory map pages %d\n", memory_map_pages);

    free_pages -= memory_map_pages;

    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    //设置前1M内存以及物理内存数组已被占用
    for (size_t i = 0;i < start_page; ++i) {
        memory_map[i] = 1;
    }

    LOGK("Total pages %d free pages %d\n", total_pages, free_pages);

    //初始化内核虚拟内存的位图，8位对齐
    u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) >> 3;
    bitmap_init(&kernel_map, (u8 *) KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, memory_map_pages);
}

//分配一页物理内存
static u32 get_page() {
    for (size_t i = start_page;i < total_pages;++i) {
        if (!memory_map[i]) {
            memory_map[i] = 1;
            free_pages -= 1;
            assert(free_pages >= 0);
            u32 page = ((u32)i) << 12;
            LOGK("GET page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of memory");
}

//释放一页物理内存
static void put_page(u32 addr) {
    ASSERT_PAGE(addr);
    
    u32 idx = IDX(addr);

    //必须在start_page后，且小于总页数
    assert(idx >= start_page && idx < total_pages);

    assert(memory_map[idx] >= 1);

    memory_map[idx] --;

    if (!memory_map[idx]) {
        free_pages ++;
    }

    assert(free_pages > 0 && free_pages < total_pages);
    LOGK("PUT page 0x%p\n", addr);
}

u32 get_cr2() {
    asm volatile("movl %cr2, %eax\n");
}

u32 get_cr3() {
    asm volatile("movl %cr3, %eax\n");
}

void set_cr3(u32 pde) {
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" :: "a"(pde));
}

//cr0最高位置为1，启用分页
static void enable_page() {
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0\n"
    );
}

//初始化页入口项
static void entry_init(page_entry_t *entry, u32 index) {
    *(u32 *)entry = 0;  //32位全写为0
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

//初始化内存映射
void mapping_init() {
    //页目录的数组
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;

    //映射内核所有的页表
    for (idx_t didx = 0;didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++) {
        //页表
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX(pte));

        for (size_t tidx = 0; tidx < 1024; tidx++, index++) {
            if (index == 0) {
                continue;
            }
            
            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1;
        }
    }

    //最后一个页表设置为页目录本身
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));
    

    set_cr3((u32)pde);

    enable_page();
}

//获取页目录 pde page directory entry
static page_entry_t *get_pde() {
    return (page_entry_t *)(0xfffff000);
}

/// @brief 获取虚拟地址对应页表
/// @param vaddr 虚拟地址
/// @param create 是否创建页表
/// @return 页表指针
static page_entry_t *get_pte(u32 vaddr, bool create) {
    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || (!create && entry->present));

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));
    
    if (!entry->present) {
        LOGK("Get and create page table entry for 0x%p\n", vaddr);
        u32 page = get_page();
        entry_init(entry, IDX(page));
        memset(table, 0, PAGE_SIZE);
    }

    return table;
}

//刷新vaddr位置的块表
static void flush_tlb(u32 vaddr) {
    asm volatile("invlpg (%0)"::"r"(vaddr) : "memory");
}

//扫描得到count个连续页
static u32 scan_page(bitmap_t *map, u32 count) {
    assert(count > 0);
    int32 index = bitmap_scan(map, count);

    if (index == EOF) {
        panic("Scan page fail");
    }

    u32 addr = PAGE(index);
    LOGK("Scan page 0x%p count %d\n", addr, count);
    return addr;
}

//重置count个连续页
static void reset_page(bitmap_t *map, u32 addr, u32 count) {
    ASSERT_PAGE(addr);
    assert(count > 0);
    u32 index = IDX(addr);
    for (size_t i = 0;i < count; i++) {
        assert(bitmap_test(map, index + i));
        bitmap_set(map, index + i, 0);
    }
}

u32 alloc_kpage(u32 count) {
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    LOGK("ALLOC kernel page 0x%p count %d\n", vaddr, count);
    return vaddr;
}

void free_kpage(u32 vaddr, u32 count) {
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
    LOGK("FREE kernel page 0x%p count %d\n", vaddr, count);
}


void link_page(u32 vaddr) {
    ASSERT_PAGE(vaddr); //必须是页

    page_entry_t *pte = get_pte(vaddr, true);    //页表
    page_entry_t *entry = &pte[TIDX(vaddr)];     //页框入口

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    if (entry->present) {
        assert(bitmap_test(map, index));
        return;
    }

    assert(!bitmap_test(map, index));
    bitmap_set(map, index, true);

    u32 paddr = get_page();
    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);

    LOGK("LINK from 0x%p to 0x%p\n", vaddr, paddr);
}

void unlink_page(u32 vaddr) {
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    if (!entry->present) {
        assert(!bitmap_test(map, index));
        return;
    }

    assert(entry->present && bitmap_test(map, index));

    entry->present = false;
    bitmap_set(map, index, false);

    u32 paddr = PAGE(entry->index);

    DEBUGK("UNLINK from 0x%p to 0x%p\n", vaddr, paddr);
    put_page(paddr);
    flush_tlb(vaddr);
}

//拷贝一页,返回拷贝后物理地址
static u32 copy_page(void *page) {
    const u32 paddr = get_page();

    page_entry_t *entry = get_pte(0, false);
    entry_init(entry, IDX(paddr));

    memcpy((void *)0, (void *)page, PAGE_SIZE);

    entry->present = false;

    return paddr;
}

page_entry_t *copy_pde() {
    task_t *task = running_task();
    //新页目录
    page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
    memcpy(pde, (void *)task->pde, PAGE_SIZE);

    //最后的页表指向自己
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(pde));

    page_entry_t *dentry;

    for (size_t didx = 2; didx < 1023; didx++) {
        dentry = &pde[didx];
        if (!dentry->present) {
            continue;
        }

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++) {
            entry = &pte[tidx];
            if (!entry->present) continue;

            assert(memory_map[entry->index] > 0);

            entry->write = false;

            memory_map[entry->index] ++;

            assert(memory_map[entry->index] < 255);
        }

        const u32 paddr = copy_page(pte);
        dentry->index = IDX(paddr);
    }
    set_cr3(task->pde);

    return pde;
}

void free_pde() {
    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    page_entry_t *pde = get_pde();
    for (size_t didx = 2; didx < 1023; didx++) {
        page_entry_t *dentry = &pde[didx];
        if (!dentry->present) continue;

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++) {
            page_entry_t *entry = &pte[tidx];
            if (!entry->present) continue;

            assert(memory_map[entry->index] > 0);
            put_page(PAGE(entry->index));
        }
        put_page(PAGE(dentry->index));
    }
    free_kpage(task->pde, 1);
    LOGK("free pages %d\n", free_pages);
}

int32 sys_brk(void *addr) {
    LOGK("task brk 0x%p\n", addr);
    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);

    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    // LOGK("MEMORY_SIZE: 0x%x, STACK_BOTTOM: 0x%x\n", KERNEL_MEMORY_SIZE, USER_STACK_BOTTOM);
    assert(KERNEL_MEMORY_SIZE <= brk && brk <= USER_STACK_BOTTOM);

    const u32 old_brk = task->brk;

    if (old_brk > brk) {
        for (u32 page = brk; page < old_brk; page += PAGE_SIZE) {
            unlink_page(page);
        }
    }
    else if (IDX(brk - old_brk) > free_pages) {
        return -1;
    }
    task->brk = brk;
    return 0;
}

typedef struct page_error_code_t {
    u8 present: 1;
    u8 write: 1;
    u8 user: 1;
    u8 reserved0: 1;
    u8 fetch: 1;
    u8 protection: 1;
    u8 shadow: 1;
    u16 reserved1: 8;
    u8 sgx: 1;
    u16 reserved2;
} _packed page_error_code_t;

//缺页异常
void page_fault(
    u32 vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags
) {
    assert(vector == 0xe);
    u32 vaddr = get_cr2();
    LOGK("fault address 0x%p\n", vaddr);

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = running_task();
    assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP);

    if (code->present) {
        assert(code->write);

        page_entry_t *pte = get_pte(vaddr, false);
        page_entry_t *entry = &pte[TIDX(vaddr)];

        assert(entry->present);
        assert(memory_map[entry->index] > 0);
        if (memory_map[entry->index] == 1) {
            entry->write = true;
            LOGK("WRITE page for 0x%p\n", vaddr);
        }
        else {
            void *page = (void *)PAGE(IDX(vaddr));
            const u32 paddr = copy_page(page);
            memory_map[entry->index]--;
            entry_init(entry, IDX(paddr));
            flush_tlb(vaddr);
            LOGK("COPY page for 0x%p\n", vaddr);
        }
        return;
    }

    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM)) {
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        // BMB;
        return;
    }

    panic("page fault!!!");
}