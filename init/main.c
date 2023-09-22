// init/main.c
#include <asm/types.h>
#include <asm/stdio.h>

extern void setup_arch(char **);
extern void show_memory_map();

void start_kernel(void)
{
	printk("Hello, OUROS.\n");
    printk("printk complete\n");
    show_memory_map();
}