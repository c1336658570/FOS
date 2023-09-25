/*
 *  linux/mm/bootmem.c
 *
 *  Copyright (C) 1999 Ingo Molnar
 *  Discontiguous memory support, Kanoj Sarcar, SGI, Nov 1999
 *
 *  simple boot-time physical memory area allocator and
 *  free memory collector. It's used to deal with reserved
 *  system memory and memory holes as well.
 */

#include <linux/bootmem.h>
#include <linux/init.h>
#include <linux/mmzone.h>
#include <asm/io.h>
#include <linux/string.h>

/*
 * Access to this subsystem has to be serialized externally. (this is
 * true for the boot process anyway)
 */
unsigned long max_low_pfn;
unsigned long min_low_pfn;
unsigned long max_pfn;


/*
 * Called once to set up the allocator itself.
 */
// 调用一次以设置分配器本身。返回值：位图大小（以字节为单位）
static unsigned long __init init_bootmem_core (pg_data_t *pgdat,
	unsigned long mapstart, unsigned long start, unsigned long end)
{
	bootmem_data_t *bdata = pgdat->bdata;
	unsigned long mapsize = ((end - start)+7)/8;	// 计算位图的大小，每个位表示一个内存页

	pgdat->node_next = pgdat_list;		// 把 pgdat_data_t 插入到 pgdat_list 链表中
	pgdat_list = pgdat;
	
	// 对齐位图大小到最接近的整数倍的sizeof(long)（通常是4或8字节）
	mapsize = (mapsize + (sizeof(long) - 1UL)) & ~(sizeof(long) - 1UL);	// 保证是 4 的倍数
	// 将物理地址转换为虚拟地址，得到位图所在的内存地址
	bdata->node_bootmem_map = phys_to_virt(mapstart << PAGE_SHIFT);	// 初始化位图的虚拟起始地址，该位图用于表示页面的使用情况
	// 记录节点可用内存的起始地址
	bdata->node_boot_start = (start << PAGE_SHIFT);	// 记录该节点的起始物理地址 pfn
	// 记录节点可用内存的最大页帧号
	bdata->node_low_pfn = end;	// 记录该节点的结束地址 pfn

	/*
	 * Initially all pages are reserved - setup_arch() has to
	 * register free RAM areas explicitly.
	 */
	memset(bdata->node_bootmem_map, 0xff, mapsize);	// 初始化位图，将所有位设为1，表示所有页都是预留的

	return mapsize;		// 返回位图的大小
}

// 初始化引导内存管理器（bootmem），用于跟踪和管理系统的物理内存
unsigned long __init init_bootmem (unsigned long start, unsigned long pages)
{
	// 设置最大和最小的低端物理页帧号
	max_low_pfn = pages;
	min_low_pfn = start;
	// 调用init_bootmem_core函数进行初始化
	return(init_bootmem_core(&contig_page_data, start, 0, pages));
}

