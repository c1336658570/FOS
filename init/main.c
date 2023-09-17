// init/main.c
#include <asm/types.h>
#include <asm/stdio.h>

void start_kernel(void)
{
	printk("Hello, OUROS.\n");
    printk("printk complete\n");
}