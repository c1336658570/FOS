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
#include <linux/debug.h>
#include <asm/bitops.h>

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

// 接受一个指向 bootmem_data_t 结构体的指针 bdata，以及起始地址 addr 和大小 size 的参数，
// 表示要释放的内存页的范围。
static void __init free_bootmem_core(bootmem_data_t *bdata, unsigned long addr, unsigned long size)
{
	unsigned long i;
	unsigned long start;
	/*
	 * round down end of usable mem, partially free pages are
	 * considered reserved.
	 */
	unsigned long sidx;
	// 根据 addr 和 size 计算出结束地址的页帧号 end 和结束索引 eidx
	unsigned long eidx = (addr + size - bdata->node_boot_start)/PAGE_SIZE;
	unsigned long end = (addr + size)/PAGE_SIZE;

	// // 检查参数是否合法
	if (!size) BUG();		// 如果 size 为 0，则触发 BUG，表示参数错误
	if (end > bdata->node_low_pfn)	// 如果结束地址超过节点可用内存的最大页帧号，则触发 BUG，表示参数错误
		BUG();

	/*
	 * Round up the beginning of the address.
	 */
	// 计算向上舍入后的起始地址 start 和起始索引偏移量 sidx。
	start = (addr + PAGE_SIZE-1) / PAGE_SIZE;	// 向上舍入到页边界
	sidx = start - (bdata->node_boot_start/PAGE_SIZE);	// 计算起始索引偏移量

	// 逐个遍历起始索引 sidx 到结束索引 eidx 之间的索引值，对应于要释放的内存页。
	// 逐个遍历页帧号，释放对应的位图位
	for (i = sidx; i < eidx; i++) {
		// 检查并清除位图中的位，如果位图已经被清除或者未被设置，则触发 BUG，表示内部错误
		if (!test_and_clear_bit(i, bdata->node_bootmem_map))
			BUG();
	}
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

// 用于释放一段引导内存页的范围。它调用了 free_bootmem_core 函数来完成实际的内存释放操作。
// 接受一个起始地址 addr 和大小 size 的参数，表示要释放的内存页的范围。
void __init free_bootmem (unsigned long addr, unsigned long size)
{
	return(free_bootmem_core(contig_page_data.bdata, addr, size));
}