#include <linux/init.h>
#include <asm/stdio.h>


void __init paging_init(void)
{
  printk("paging_init start\n");

	// pagetable_init();
  
  printk("paging_init end\n");
}