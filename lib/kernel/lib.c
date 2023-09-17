#include <asm/stdio.h>
#include <linux/string.h>
#include <asm/io.h>

/*
颜色生成方法
MAKE_COLOR(BLUE, RED)
MAKE_COLOR(BLACK, RED) | BRIGHT
MAKE_COLOR(BLACK, RED) | BRIGHT | FLASH
*/
#define TEXT_BLACK   0x0     /* 0000 */   // 文本颜色: 黑色
#define TEXT_WHITE   0x7     /* 0111 */   // 文本颜色: 白色
#define TEXT_RED     0x4     /* 0100 */   // 文本颜色: 红色
#define TEXT_GREEN   0x2     /* 0010 */   // 文本颜色: 绿色
#define TEXT_BLUE    0x1     /* 0001 */   // 文本颜色: 蓝色
#define TEXT_FLASH   0x80    /* 1000 0000 */  // 使文本闪烁
#define TEXT_BRIGHT  0x08    /* 0000 1000 */  // 使文本更亮
// 用于生成VGA的颜色代码。x是背景颜色，y是前景色
#define	MAKE_COLOR(x,y)	((x<<4) | y) /* MAKE_COLOR(Background,Foreground) */

#define DISPLAY_VRAM 0xc00b8000     // 文本模式的起始物理地址

// CRT控制器的地址和数据端口号（光标位置寄存器）
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */

// VGA寄存器的索引，用于设置视频内存的起始地址和光标位置
// START_ADDR_H和START_ADDR_L
// 这两个宏定义代表的是视频内存起始地址的高8位和低8位在CRT控制器中的索引。
// 当你想滚动屏幕内容时，可以通过更改这两个寄存器的值来更改视频内存的起始地址。
// 例如，如果你想向上滚动一行，你可以将视频内存的起始地址增加一个行的长度，这样屏幕上显示的内容就会向上移动一行。
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
// 这两个宏定义代表的是光标位置的高8位和低8位在CRT控制器中的索引。
// 光标是屏幕上一个闪烁的下划线或方块，表示下一个字符将被写入的位置。通过更改这两个寄存器的值，你可以移动光标到屏幕上的任何位置。
#define	CURSOR_H	    0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	    0xF	/* reg index of cursor position (LSB) */

// 颜色视频内存的基址
#define	V_MEM_BASE	    DISPLAY_VRAM	/* base of color video memory */

// 用于屏幕滚动的方向
#define SCREEN_UP -1
#define SCREEN_DOWN 1

// VGA文本模式的屏幕宽度和高度（以字符为单位）
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// 默认颜色: 黑色背景与白色前景
#define COLOR_DEFAULT	(MAKE_COLOR(TEXT_BLACK, TEXT_WHITE))

int cursor_x = 0, cursor_y = 0;

void outsb(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsb": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void outsw(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsw": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void outsl(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsl": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}


void insb(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insb": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void insw(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insw": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void insl(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insl": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}

unsigned char inb(unsigned short port)
{
    unsigned char _v;
     // N为立即数约束，它表示0～255的立即数，对端口指定N表示0～255，d表示用dx存储端口号，%b0表示对应al，%w1表示对应dx
    __asm__ __volatile__ ("inb %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned short inw(unsigned short port)
{
    unsigned short _v;
    __asm__ __volatile__ ("inw %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned int inl(unsigned short port)
{
    unsigned int _v;
    __asm__ __volatile__ ("inl %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}

unsigned char inb_p(unsigned short port)
{
    unsigned char _v;
    __asm__ __volatile__ ("inb %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned short inw_p(unsigned short port)
{
    unsigned short _v;
    __asm__ __volatile__ ("inw %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned int inl_p(unsigned short port)
{
    unsigned int _v;
    __asm__ __volatile__ ("inl %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}

void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__ ("outb %b0,%w1": : "a" (value), "Nd" (port));
}

void outw(unsigned short value, unsigned short port)
{
    __asm__ __volatile__ ("outw %w0,%w1": : "a" (value), "Nd" (port));
}

void outl(unsigned int value, unsigned short port)
{
    __asm__ __volatile__ ("outl %0,%w1": : "a" (value), "Nd" (port));
}

void outb_p(unsigned char value, unsigned short port)
{
    __asm__ __volatile__ ("outb %b0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}

void outw_p(unsigned short value, unsigned short port)
{
    __asm__ __volatile__ ("outw %w0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}

void outl_p(unsigned int value, unsigned short port)
{
    __asm__ __volatile__ ("outl %0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}

static void set_cursor(unsigned short cursor)
{
    //设置光标位置 0-2000
    outb(CURSOR_H, CRTC_ADDR_REG);			//光标高位
    outb((cursor >> 8) & 0xFF, CRTC_DATA_REG);
    outb(CURSOR_L,CRTC_ADDR_REG);			//光标低位
    outb(cursor & 0xFF, CRTC_DATA_REG);
}

/**
 * flush - 刷新光标和起始位置
 * @console: 控制台
 */
static void flush()
{
    /* 计算光标位置，并设置 */
    set_cursor(cursor_y * SCREEN_WIDTH + cursor_x);
}

/**
 * scroll_sceen - 滚屏
 * @console: 控制台
 * @direction: 滚动方向
 *             - SCREEN_UP: 向上滚动
 *             - SCREEN_DOWN: 向下滚动
 * 
 */
static void scroll_sceen(int direction)
{
    unsigned char *vram = (unsigned char *)(V_MEM_BASE);
    int i;

    if(direction == SCREEN_UP){
        /* 起始地址 */
        for (i = SCREEN_WIDTH * 2 * 24; i > SCREEN_WIDTH * 2; i -= 2) {
            vram[i] = vram[i - SCREEN_WIDTH * 2];
            vram[i + 1] = vram[i + 1 - SCREEN_WIDTH * 2];
        }
        for (i = 0; i < SCREEN_WIDTH * 2; i += 2) {
            vram[i] = '\0';
            vram[i + 1] = COLOR_DEFAULT;
        }

    }else if(direction == SCREEN_DOWN){
        /* 起始地址 */
        for (i = 0; i < SCREEN_WIDTH * 2 * 24; i += 2) {
            vram[i] = vram[i + SCREEN_WIDTH * 2];
            vram[i + 1] = vram[i + 1 + SCREEN_WIDTH * 2];
        }
        for (i = SCREEN_WIDTH * 2 * 24; i < SCREEN_WIDTH * 2 * 25; i += 2) {
            vram[i] = '\0';
            vram[i + 1] = COLOR_DEFAULT;
        }
        cursor_y--;
    }
    flush();
}

/**
 * put_char - 控制台上输出一个字符
 * @console: 控制台
 * @ch: 字符
 */
void put_char(char ch)
{
    char *vram = (char *)(V_MEM_BASE +
                          (cursor_y * SCREEN_WIDTH + cursor_x) *2) ;
    switch(ch){
        case '\r':
            break;
        case '\n':
            // 如果是回车，那还是要把回车写进去
            *vram++ = '\0';
            *vram = COLOR_DEFAULT;
            cursor_x = 0;
            cursor_y++;

            break;
        case '\b':
            if (cursor_x >= 0 && cursor_y >= 0) {
                cursor_x--;
                /* 调整为上一行尾 */
                if (cursor_x < 0) {
                    cursor_x = SCREEN_WIDTH - 1;
                    cursor_y--;
                    /* 对y进行修复 */
                    if (cursor_y < 0)
                        cursor_y = 0;
                }
                *(vram-2) = '\0';
                *(vram-1) = COLOR_DEFAULT;
            }
            break;
        default:
            *vram++ = ch;
            *vram = COLOR_DEFAULT;

            cursor_x++;
            if (cursor_x > SCREEN_WIDTH - 1) {
                cursor_x = 0;
                cursor_y++;
            }
            break;
    }
    /* 滚屏 */
    while (cursor_y > SCREEN_HEIGHT - 1) {
        scroll_sceen(SCREEN_DOWN);
    }

    flush();
}

void print_str(char *str)
{
    while (*str) {
        put_char(*str);
        str++;
    }
}

void print_int(int num)
{
    char buf[24];
    memset(buf, 0, 24);
    itoa(num, buf, 10);
    print_str(buf);
}

void print_hex(unsigned int num)
{
    char buf[24];
    memset(buf, 0, 24);
    uitoa(num, buf, 16);
    print_str(buf);
}