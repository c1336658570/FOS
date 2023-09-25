/*
 * Discontiguous memory support, Kanoj Sarcar, SGI, Nov 1999
 */
#ifndef _LINUX_BOOTMEM_H
#define _LINUX_BOOTMEM_H

#include <linux/init.h>

/*
 *  simple boot-time physical memory area allocator.
 */

extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;
extern unsigned long max_pfn;     //最大物理页号

/*
 * node_bootmem_map is a map pointer - the bits represent all physical 
 * memory pages (including holes) on the node.
 */
typedef struct bootmem_data {
	unsigned long node_boot_start;	// 节点在内存中的起始物理地址。它表示分配给节点的内存范围的开始位置。
	unsigned long node_low_pfn;			// 节点的最低页帧号（PFN）。PFN表示内存页的物理页帧号码。该字段表示节点的物理内存范围从低PFN到高PFN。
	void *node_bootmem_map;					// 节点的引导内存映射的指针。引导内存映射是一个位图，其中每个位表示一个物理内存页，包括空洞（未分配或保留的页）。该映射用于跟踪引导过程中内存页的分配状态。
	unsigned long last_offset;			// 上一次偏移量，表示上一次内存分配的偏移量。它表示上一次内存分配发生的引导内存映射中的偏移量。该字段用于通过从上一次的偏移量开始搜索而不是扫描整个映射来优化后续的内存分配。
	unsigned long last_pos;					// 上一次位置，表示上一次内存分配的位置。该字段通过提供搜索的起始位置来帮助优化内存分配。
} bootmem_data_t;

extern unsigned long __init bootmem_bootmap_pages (unsigned long);
extern unsigned long __init init_bootmem (unsigned long addr, unsigned long memend);
extern void __init reserve_bootmem (unsigned long addr, unsigned long size);
extern void __init free_bootmem (unsigned long addr, unsigned long size);
extern void * __init __alloc_bootmem (unsigned long size, unsigned long align, unsigned long goal);
#define alloc_bootmem(x) \
	__alloc_bootmem((x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
#define alloc_bootmem_low(x) \
	__alloc_bootmem((x), SMP_CACHE_BYTES, 0)
#define alloc_bootmem_pages(x) \
	__alloc_bootmem((x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
#define alloc_bootmem_low_pages(x) \
	__alloc_bootmem((x), PAGE_SIZE, 0)
extern unsigned long __init free_all_bootmem (void);

// extern unsigned long __init init_bootmem_node (pg_data_t *pgdat, unsigned long freepfn, unsigned long startpfn, unsigned long endpfn);
// extern void __init reserve_bootmem_node (pg_data_t *pgdat, unsigned long physaddr, unsigned long size);
// extern void __init free_bootmem_node (pg_data_t *pgdat, unsigned long addr, unsigned long size);
// extern unsigned long __init free_all_bootmem_node (pg_data_t *pgdat);
// extern void * __init __alloc_bootmem_node (pg_data_t *pgdat, unsigned long size, unsigned long align, unsigned long goal);
#define alloc_bootmem_node(pgdat, x) \
	__alloc_bootmem_node((pgdat), (x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
#define alloc_bootmem_pages_node(pgdat, x) \
	__alloc_bootmem_node((pgdat), (x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
#define alloc_bootmem_low_pages_node(pgdat, x) \
	__alloc_bootmem_node((pgdat), (x), PAGE_SIZE, 0)

#endif