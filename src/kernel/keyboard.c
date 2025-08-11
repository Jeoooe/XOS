#include <xos/interrupt.h>
#include <xos/io.h>
#include <xos/assert.h>
#include <xos/debug.h>
#include <xos/fifo.h>
#include <xos/mutex.h>
#include <xos/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CTRL_PORT 0x64

#define KEYBOARD_CMD_LED 0xED
#define KEYBOARD_CMD_ACK 0xFA   //表示命令已收到

#define INV 0   //不可见字符

#define CODE_PRINT_SCREEN_DOWN 0xB7     

/**
 * XT键盘扫描码（第一套）通码枚举
 * 注：这是键盘按下时发送的扫描码（通码，Make code）
 * 部分键的扫描码在XT键盘上不存在，已标注为保留
 */
typedef enum {
    KEY_NONE        = 0x00,   // 无键

    // 主键盘区
    KEY_ESC         = 0x01,
    KEY_1           = 0x02,
    KEY_2           = 0x03,
    KEY_3           = 0x04,
    KEY_4           = 0x05,
    KEY_5           = 0x06,
    KEY_6           = 0x07,
    KEY_7           = 0x08,
    KEY_8           = 0x09,
    KEY_9           = 0x0A,
    KEY_0           = 0x0B,
    KEY_MINUS       = 0x0C,    // - (减号)
    KEY_EQUAL       = 0x0D,    // = (等号)
    KEY_BACKSPACE   = 0x0E,
    KEY_TAB         = 0x0F,
    KEY_Q           = 0x10,
    KEY_W           = 0x11,
    KEY_E           = 0x12,
    KEY_R           = 0x13,
    KEY_T           = 0x14,
    KEY_Y           = 0x15,
    KEY_U           = 0x16,
    KEY_I           = 0x17,
    KEY_O           = 0x18,
    KEY_P           = 0x19,
    KEY_BRACKET_L   = 0x1A,    // [
    KEY_BRACKET_R   = 0x1B,    // ]
    KEY_ENTER       = 0x1C,
    KEY_CTRL_L      = 0x1D,
    KEY_A           = 0x1E,
    KEY_S           = 0x1F,
    KEY_D           = 0x20,
    KEY_F           = 0x21,
    KEY_G           = 0x22,
    KEY_H           = 0x23,
    KEY_J           = 0x24,
    KEY_K           = 0x25,
    KEY_L           = 0x26,
    KEY_SEMICOLON   = 0x27,    // ;
    KEY_QUOTE       = 0x28,    // '
    KEY_BACKQUOTE   = 0x29,    // ` (反引号)
    KEY_SHIFT_L     = 0x2A,
    KEY_BACKSLASH   = 0x2B,    // \ ;;;;
    KEY_Z           = 0x2C,
    KEY_X           = 0x2D,
    KEY_C           = 0x2E,
    KEY_V           = 0x2F,
    KEY_B           = 0x30,
    KEY_N           = 0x31,
    KEY_M           = 0x32,
    KEY_COMMA       = 0x33,    // ,
    KEY_POINT       = 0x34,    // .
    KEY_SLASH       = 0x35,    // /
    KEY_SHIFT_R     = 0x36,
    KEY_STAR        = 0x37,    // 小键盘 *
    KEY_ALT_L       = 0x38,
    KEY_SPACE       = 0x39,
    KEY_CAPSLOCK    = 0x3A,
    KEY_F1          = 0x3B,
    KEY_F2          = 0x3C,
    KEY_F3          = 0x3D,
    KEY_F4          = 0x3E,
    KEY_F5          = 0x3F,
    KEY_F6          = 0x40,
    KEY_F7          = 0x41,
    KEY_F8          = 0x42,
    KEY_F9          = 0x43,
    KEY_F10         = 0x44,
    KEY_NUMLOCK     = 0x45,
    KEY_SCRLOCK     = 0x46,

    // 小键盘区
    KEY_PAD_7       = 0x47,
    KEY_PAD_8       = 0x48,
    KEY_PAD_9       = 0x49,
    KEY_PAD_MINUS   = 0x4A,
    KEY_PAD_4       = 0x4B,
    KEY_PAD_5       = 0x4C,
    KEY_PAD_6       = 0x4D,
    KEY_PAD_PLUS    = 0x4E,
    KEY_PAD_1       = 0x4F,
    KEY_PAD_2       = 0x50,
    KEY_PAD_3       = 0x51,
    KEY_PAD_0       = 0x52,
    KEY_PAD_POINT   = 0x53,    // 小键盘 .

    // 以下为XT键盘不存在的键（保留）
    KEY_54          = 0x54,
    KEY_55          = 0x55,
    KEY_56          = 0x56,
    KEY_F11         = 0x57,    // XT键盘无F11/F12
    KEY_F12         = 0x58,
    KEY_59          = 0x59,
    KEY_WIN_L       = 0x5A,
    KEY_WIN_R       = 0x5B,
    KEY_CLIPBOARD   = 0x5C,
    KEY_5D          = 0x5D,
    KEY_5E          = 0x5E,

    //自定义按键
    KEY_PRINT_SCREEN= 0x5F,
    
    // 其他特殊键（XT键盘无这些键）
    KEY_CTRL_R      = 0xE0,    // XT键盘无独立右Ctrl
    KEY_ALT_R       = 0xE0,    // XT键盘无独立右Alt
    KEY_HOME        = 0xE0,
    KEY_UP          = 0xE0,
    KEY_PAGEUP      = 0xE0,
    KEY_LEFT        = 0xE0,
    KEY_RIGHT       = 0xE0,
    KEY_END         = 0xE0,
    KEY_DOWN        = 0xE0,
    KEY_PAGEDOWN    = 0xE0,
    KEY_INSERT      = 0xE0,
    KEY_DELETE      = 0xE0,
    KEY_LEFTMETA    = 0xE0,    // Windows键
    KEY_RIGHTMETA   = 0xE0,
    KEY_COMPOSE     = 0xE0     // 菜单键
} KEY;

static char keymap[][4] = {
    /* 扫描码 {无shift组合, shift组合, 按键是否按下, 扩展键是否按下} */
    /* 0x00 */ {INV,      INV,      false, false},   // NULL
    /* 0x01 */ {0x1b,     0x1b,     false, false},   // ESC
    /* 0x02 */ {'1',      '!',      false, false},   // 1 !
    /* 0x03 */ {'2',      '@',      false, false},   // 2 @
    /* 0x04 */ {'3',      '#',      false, false},   // 3 #
    /* 0x05 */ {'4',      '$',      false, false},   // 4 $
    /* 0x06 */ {'5',      '%',      false, false},   // 5 %
    /* 0x07 */ {'6',      '^',      false, false},   // 6 ^
    /* 0x08 */ {'7',      '&',      false, false},   // 7 &
    /* 0x09 */ {'8',      '*',      false, false},   // 8 *
    /* 0x0A */ {'9',      '(',      false, false},   // 9 (
    /* 0x0B */ {'0',      ')',      false, false},   // 0 )
    /* 0x0C */ {'-',      '_',      false, false},   // - _
    /* 0x0D */ {'=',      '+',      false, false},   // = +
    /* 0x0E */ {'\b',     '\b',     false, false},   // Backspace
    /* 0x0F */ {'\t',     '\t',     false, false},   // Tab
    /* 0x10 */ {'q',      'Q',      false, false},   // Q
    /* 0x11 */ {'w',      'W',      false, false},   // W
    /* 0x12 */ {'e',      'E',      false, false},   // E
    /* 0x13 */ {'r',      'R',      false, false},   // R
    /* 0x14 */ {'t',      'T',      false, false},   // T
    /* 0x15 */ {'y',      'Y',      false, false},   // Y
    /* 0x16 */ {'u',      'U',      false, false},   // U
    /* 0x17 */ {'i',      'I',      false, false},   // I
    /* 0x18 */ {'o',      'O',      false, false},   // O
    /* 0x19 */ {'p',      'P',      false, false},   // P
    /* 0x1A */ {'[',      '{',      false, false},   // [ {
    /* 0x1B */ {']',      '}',      false, false},   // ] }
    /* 0x1C */ {'\n',     '\n',     false, false},   // Enter
    /* 0x1D */ {INV,      INV,      false, false},   // Left Ctrl (无字符)
    /* 0x1E */ {'a',      'A',      false, false},   // A
    /* 0x1F */ {'s',      'S',      false, false},   // S
    /* 0x20 */ {'d',      'D',      false, false},   // D
    /* 0x21 */ {'f',      'F',      false, false},   // F
    /* 0x22 */ {'g',      'G',      false, false},   // G
    /* 0x23 */ {'h',      'H',      false, false},   // H
    /* 0x24 */ {'j',      'J',      false, false},   // J
    /* 0x25 */ {'k',      'K',      false, false},   // K
    /* 0x26 */ {'l',      'L',      false, false},   // L
    /* 0x27 */ {';',      ':',      false, false},   // ; :
    /* 0x28 */ {'\'',     '\"',     false, false},   // ' "
    /* 0x29 */ {'`',      '~',      false, false},   // ` ~
    /* 0x2A */ {INV,      INV,      false, false},   // Left Shift (无字符)
    /* 0x2B */ {'\\',     '|',      false, false},   // \ |
    /* 0x2C */ {'z',      'Z',      false, false},   // Z
    /* 0x2D */ {'x',      'X',      false, false},   // X
    /* 0x2E */ {'c',      'C',      false, false},   // C
    /* 0x2F */ {'v',      'V',      false, false},   // V
    /* 0x30 */ {'b',      'B',      false, false},   // B
    /* 0x31 */ {'n',      'N',      false, false},   // N
    /* 0x32 */ {'m',      'M',      false, false},   // M
    /* 0x33 */ {',',      '<',      false, false},   // , <
    /* 0x34 */ {'.',      '>',      false, false},   // . >
    /* 0x35 */ {'/',      '?',      false, false},   // / ?
    /* 0x36 */ {INV,      INV,      false, false},   // Right Shift (无字符)
    /* 0x37 */ {'*',      '*',      false, false},   // 小键盘 *
    /* 0x38 */ {INV,      INV,      false, false},   // Left Alt (无字符)
    /* 0x39 */ {' ',      ' ',      false, false},   // Space
    /* 0x3A */ {INV,      INV,      false, false},   // Caps Lock (无字符)
    /* 0x3B */ {INV,      INV,      false, false},   // F1 (无字符)
    /* 0x3C */ {INV,      INV,      false, false},   // F2 (无字符)
    /* 0x3D */ {INV,      INV,      false, false},   // F3 (无字符)
    /* 0x3E */ {INV,      INV,      false, false},   // F4 (无字符)
    /* 0x3F */ {INV,      INV,      false, false},   // F5 (无字符)
    /* 0x40 */ {INV,      INV,      false, false},   // F6 (无字符)
    /* 0x41 */ {INV,      INV,      false, false},   // F7 (无字符)
    /* 0x42 */ {INV,      INV,      false, false},   // F8 (无字符)
    /* 0x43 */ {INV,      INV,      false, false},   // F9 (无字符)
    /* 0x44 */ {INV,      INV,      false, false},   // F10 (无字符)
    /* 0x45 */ {INV,      INV,      false, false},   // Num Lock (无字符)
    /* 0x46 */ {INV,      INV,      false, false},   // Scroll Lock (无字符)
    /* 0x47 */ {'7',      '7',      false, false},   // 小键盘 7 (Home)
    /* 0x48 */ {'8',      '8',      false, false},   // 小键盘 8 (↑)
    /* 0x49 */ {'9',      '9',      false, false},   // 小键盘 9 (PgUp)
    /* 0x4A */ {'-',      '-',      false, false},   // 小键盘 -
    /* 0x4B */ {'4',      '4',      false, false},   // 小键盘 4 (←)
    /* 0x4C */ {'5',      '5',      false, false},   // 小键盘 5
    /* 0x4D */ {'6',      '6',      false, false},   // 小键盘 6 (→)
    /* 0x4E */ {'+',      '+',      false, false},   // 小键盘 +
    /* 0x4F */ {'1',      '1',      false, false},   // 小键盘 1 (End)
    /* 0x50 */ {'2',      '2',      false, false},   // 小键盘 2 (↓)
    /* 0x51 */ {'3',      '3',      false, false},   // 小键盘 3 (PgDn)
    /* 0x52 */ {'0',      '0',      false, false},   // 小键盘 0 (Ins)
    /* 0x53 */ {'.',      '.',      false, false},   // 小键盘 . (Del)
    /* 0x54 */ {INV,      INV,      false, false},   // (保留)
    /* 0x55 */ {INV,      INV,      false, false},   // (保留)
    /* 0x56 */ {INV,      INV,      false, false},   // (保留)
    /* 0x57 */ {INV,      INV,      false, false},   // F11 (XT键盘无)
    /* 0x58 */ {INV,      INV,      false, false},   // F12 (XT键盘无)
    /* 0x59 */ {INV,      INV,      false, false},   // (保留)
    /* 0x5A */ {INV,      INV,      false, false},   // (保留)
    /* 0x5B */ {INV,      INV,      false, false},   // (保留)
    /* 0x5C */ {INV,      INV,      false, false},   // (保留)
    /* 0x5D */ {INV,      INV,      false, false},   // (保留)
    /* 0x5E */ {INV,      INV,      false, false},   // (保留)

    //强制定义 print screen 为0x5F
    /* 0x5F */ {INV,      INV,      false, false},   // PrintScreen
};

static lock_t lock;             //进程锁
static task_t *waiter;          //等待输入的任务

#define BUFFER_SIZE 64          //输入缓冲区大小
static char buf[BUFFER_SIZE];   //输入缓冲区    
static fifo_t fifo;             


static bool capslock_state;     //大写锁定
static bool scrlock_state;      //滚动锁定
static bool numlock_state;      //数字锁定
static bool extcode_state;      //扩展码状态

#define ctrl_state (keymap[KEY_CTRL_L][2] || keymap[KEY_CTRL_L][3])
#define alt_state (keymap[KEY_ALT_L][2] || keymap[KEY_ALT_L][3])
#define shift_state (keymap[KEY_SHIFT_L][2] || keymap[KEY_SHIFT_L][2])

static void keyboard_wait() {
    u8 state;
    do {
        state = inb(KEYBOARD_CTRL_PORT);
    } while (state & 0x02); //等待缓冲区为空
}

static void keyboard_ack() {
    u8 state;
    do {
        state = inb(KEYBOARD_DATA_PORT);
    } while (state != KEYBOARD_CMD_ACK);
}

static void set_leds() {
    u8 leds = (capslock_state << 2) | (numlock_state << 1) | scrlock_state;
    
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, KEYBOARD_CMD_LED);

    keyboard_ack();
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, leds);
    keyboard_ack();
}

void keyboard_handler(int vector) {
    assert(vector == 0x21);
    send_eoi(vector);   //中断处理完成信号

    u16 scancode = inb(KEYBOARD_DATA_PORT);
    u8 ext = 2;

    //扩展码字节
    if (scancode == 0xe0) {
        extcode_state = true;
        return;
    }

    if (extcode_state) {
        ext = 3;
        //添加e0前缀
        scancode |= 0xe000;
        extcode_state = false;
    }

    //通码
    u16 makecode = (scancode & 0x7f);
    //特殊的自定义字符CODE_PRINT_SCREEN_DOWN
    if (makecode == CODE_PRINT_SCREEN_DOWN) {
        makecode = KEY_PRINT_SCREEN;
    }

    //非法
    if (makecode > KEY_PRINT_SCREEN) {
        return;
    }

    //断码
    bool breakcode = ((scancode & 0x0080) != 0);
    if (breakcode) {
        keymap[makecode][ext] = false;
        return;
    }

    keymap[makecode][ext] = true;

    //是否需要LED灯
    bool led = false;
    if (makecode == KEY_NUMLOCK) {
        numlock_state = !numlock_state;
        led = true;
    }
    else if (makecode == KEY_CAPSLOCK) {
        capslock_state = !capslock_state;
        led = true;
    }
    else if (makecode == KEY_SCRLOCK) {
        scrlock_state = !scrlock_state;
        led = true;
    }

    if (led) {
        set_leds();
    }

    //shift
    bool shift = false;
    if (capslock_state && ('a' <= keymap[makecode][0] && keymap[makecode][0] <= 'z')) {
        shift = !shift;
    }
    if (shift_state) {
        shift = !shift;
    }

    //按键ASCII码
    char ch = 0;
    //特殊处理 [/?],扩展码与普通码一致
    if (ext == 3 && (makecode != KEY_SLASH)) {
        ch = keymap[makecode][1];
    }
    //处理小键盘的[/]键
    else if (ext == 3 && (makecode == KEY_SLASH)) {
        ch = keymap[makecode][0];
    }
    else {
        ch = keymap[makecode][shift];
    }

    if (ch == INV) {
        return;
    }

    //ch是输入的字符,可见字符
    // LOGK("keydown %c \n", ch);
    fifo_put(&fifo, ch);
    if (waiter != NULL) {
        task_unblock(waiter);
        waiter = NULL;
    }
}

/// @brief 从键盘读取count个数据到buf,不可中断函数
/// @return count
u32 keyboard_read(char *buf, u32 count) {
    lock_acquire(&lock);
    int nr = 0;
    while (nr < count) {
        while (fifo_empty(&fifo)) {
            waiter = running_task();
            task_block(waiter, NULL, TASK_WAITING);
        }
        buf[nr++] = fifo_get(&fifo);
    }
    lock_release(&lock);
    return count;
}

void keyboard_init() {
    numlock_state = false;
    scrlock_state = false;
    capslock_state = false;
    extcode_state = false;

    fifo_init(&fifo, buf, BUFFER_SIZE);
    lock_init(&lock);
    waiter = NULL;

    set_leds();

    set_interupt_handler(IRQ_KEYBOARD, keyboard_handler);
    set_interupt_mask(IRQ_KEYBOARD, true);
}