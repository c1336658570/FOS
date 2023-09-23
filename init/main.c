// init/main.c
#include <asm/stdio.h>
#include <asm/types.h>
#include <linux/init.h>

extern void __init setup_arch();
extern uint8_t _start[];
extern uint8_t _end[];

void start_kernel(void) {
  printk("Hello, OUROS.\n");
  printk("printk complete\n");
  printk("kernel in memory start: 0x%08X\n", _start);
  printk("kernel in memory end:   0x%08X\n", _end);
  setup_arch();
}