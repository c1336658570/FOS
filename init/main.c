// init/main.c
#include <asm/types.h>
#include <asm/stdio.h>

extern void setup_arch(char **);

void start_kernel(void)
{
	printk("Hello, OUROS.\n");
    printk("printk complete\n");
    setup_arch(NULL);
}