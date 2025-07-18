extern void console_init();
extern void gdt_init();
extern interrupt_init();
extern task_init();
extern hang();
extern clock_init();
extern time_init();

void kernel_init() {
    console_init();
    gdt_init();
    interrupt_init();

    clock_init();
    time_init();

    asm volatile("sti");
    hang();
    return; 
}