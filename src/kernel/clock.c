#include <xos/assert.h>
#include <xos/types.h>
#include <xos/debug.h>
#include <xos/interrupt.h>
#include <xos/io.h>
#include <xos/task.h>

#define PIT_CHAN0_REG 0x40
#define PIT_CHAN2_REG 0x42
#define PIT_CTRL_REG 0x43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0x61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)

u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

u32 volatile beeping = 0;

void start_beep() {
    if (!beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG) | 0b11);
    }
    beeping = jiffies + 5;
}

void stop_beep() {
    if (beeping && jiffies > beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);
        beeping = 0;
    }
}

//时钟中断函数
void clock_handler(int vector) {
    assert(vector == 0x20);
    send_eoi(vector);
    stop_beep();

    ++jiffies;
    // DEBUGK("clock jiffies %d... \n", jiffies);
    task_t *task = running_task();
    assert(task->magic == XOS_MAGIC);

    task->jiffies = jiffies;
    task->ticks --;
    if (!task->ticks) {
        task->ticks = task->priority;
        schedule();
    }
}

void pit_init() {
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff );

    outb(PIT_CTRL_REG, 0b10110110);
    outb(PIT_CHAN2_REG, (u8)BEEP_COUNTER);
    outb(PIT_CHAN2_REG, (u8)(BEEP_COUNTER >> 8));
}

void clock_init() {
    pit_init();
    set_interupt_handler(IRQ_CLOCK, clock_handler);
    set_interupt_mask(IRQ_CLOCK, true);
}