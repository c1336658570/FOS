#include <asm/e820.h>
#include <asm/stdio.h>
#include <linux/debug.h>
#include <linux/init.h>
#include <linux/multiboot.h>
#include <linux/string.h>
#include <asm/page.h>
#include <linux/bootmem.h>
#include <linux/kernel.h>
#include <asm/pgtable.h>
#include <linux/bootmem.h>

// 用户定义的 highmem_pages 大小（高端内存的页数）
static unsigned int highmem_pages __initdata = -1;

struct e820map e820;
struct e820map biosmap;

extern char _text, _etext, _edata, _end;

// 打印系统的内存映射
void show_memory_map() {
  // 从全局的 multiboot 指针中获取内存映射的地址和长度
  uint32_t mmap_addr = glb_mboot_ptr->mmap_addr;
  uint32_t mmap_length = glb_mboot_ptr->mmap_length;

  printk("Memory map:\n");

  // 定义一个指向内存映射条目的指针
  mmap_entry_t *mmap = (mmap_entry_t *)mmap_addr;
  // 遍历所有的内存映射条目
  for (mmap = (mmap_entry_t *)mmap_addr;
       (uint32_t)mmap < mmap_addr + mmap_length; mmap++) {
    // 打印每个内存映射条目的基地址、长度和类型
    printk("base_addr = 0x%X%08X, length = 0x%X%08X, type = 0x%X\n",
           (uint32_t)mmap->base_addr_high, (uint32_t)mmap->base_addr_low,
           (uint32_t)mmap->length_high, (uint32_t)mmap->length_low,
           (uint32_t)mmap->type);
  }
}

// 初始化biosmap这个结构体，根据grub提供的内存信息
void __init biosmap_init() {
  // printk("biosmap_init start\n");

  // 从全局的 multiboot 指针中获取内存映射的地址和长度
  uint32_t mmap_addr = glb_mboot_ptr->mmap_addr;
  uint32_t mmap_length = glb_mboot_ptr->mmap_length;

  int cnt = 0;
  // 定义一个指向内存映射条目的指针
  mmap_entry_t *mmap = (mmap_entry_t *)mmap_addr;
  for (; (uint32_t)mmap < mmap_addr + mmap_length; ++mmap) {
    // 将当前条目的信息存储到 biosmap 数据结构中
    biosmap.map[cnt].addr = mmap->base_addr_low;
    biosmap.map[cnt].size = mmap->length_low;
    biosmap.map[cnt].type = mmap->type;
    cnt++;
  }
  biosmap.nr_map = cnt;  // 设置 biosmap 的条目数量

  // printk("biosmap_init end\n");
}

// 这个函数的目的是处理BIOS提供的E820内存映射中可能存在的重叠区域，并创建一个新的、没有重叠的内存映射
static int __init sanitize_e820_map(struct e820entry *biosmap, int *pnr_map) {
  // 对BIOS提供的E820内存映射进行清理和整合。E820内存映射是BIOS提供的一种方式，
  // 用于描述系统中的物理内存布局。这个映射可能包含多个条目，每个条目描述一个内存区域的起始地址、
  // 大小和类型（例如，可用、保留、ACPI可回收等）。

  // printk("sanitize_e820_map start\n");

  // 用于记录内存映射变化的结构体
  struct change_member {
    // 指向原始 BIOS 条目的指针
    struct e820entry *pbios; /* pointer to original bios entry */
    // 此变化点的地址
    unsigned long long addr; /* address for this change point */
  };

  // 为了处理可能的内存区域重叠，我们需要一些临时数据结构
  struct change_member change_point_list[2 * E820MAX];
  struct change_member *change_point[2 * E820MAX];
  struct e820entry *overlap_list[E820MAX];  // 跟踪当前重叠的 BIOS 条目
  struct e820entry new_bios[E820MAX];
  struct change_member *change_tmp;
  unsigned long current_type, last_type;
  unsigned long long last_addr;
  int chgidx, still_changing;
  int overlap_entries;
  int new_bios_entry;
  int old_nr, new_nr, chg_nr;
  int i;

  /* if there's only one memory region, don't bother */
  if (*pnr_map < 2)  // 如果只有一个内存区域，不做任何处理
    return -1;

  old_nr = *pnr_map;

  /* bail out if we find any unreasonable addresses in bios map */
  for (i = 0; i < old_nr; i++)  // 检查BIOS映射中是否有不合理的地址
    if (biosmap[i].addr + biosmap[i].size < biosmap[i].addr) 
      return -1;

  /* create pointers for initial change-point information (for sorting) */
  for (i = 0; i < 2 * old_nr; i++)  // 创建初始变化点信息的指针（用于排序）
    change_point[i] = &change_point_list[i];

  /* record all known change-points (starting and ending addresses),
     omitting those that are for empty memory regions */
  // 记录所有已知的变化点（开始和结束地址），忽略那些表示空内存区域的
  chgidx = 0;
  for (i = 0; i < old_nr; i++) {  // change_point 每两项对应 biosmap 的一项。两项中的前一项记录起始地址，后一项记录结束地址
    if (biosmap[i].size != 0) {
      change_point[chgidx]->addr = biosmap[i].addr;
      change_point[chgidx++]->pbios = &biosmap[i];
      change_point[chgidx]->addr = biosmap[i].addr + biosmap[i].size;
      change_point[chgidx++]->pbios = &biosmap[i];
    }
  }

  // 真正的变化点数量
  chg_nr = chgidx; /* true number of change-points */

  /* sort change-point list by memory addresses (low -> high) */
  // 按内存地址对变化点列表进行排序（从低到高）
  still_changing = 1;   // 用于标记是否还需要进行交换操作的标志位
  while (still_changing) {    // 进入循环直到没有需要交换的元素为止
    still_changing = 0;       // 每次循环开始时，假设没有需要交换的元素
    for (i = 1; i < chg_nr; i++) {
      /* if <current_addr> > <last_addr>, swap */
      /* or, if current=<start_addr> & last=<end_addr>, swap */
      // 当前地址小于前一个地址，或者当前地址等于前一个地址并且前一个是结尾，就交换
      if ((change_point[i]->addr < change_point[i - 1]->addr) ||
          ((change_point[i]->addr == change_point[i - 1]->addr) &&
           (change_point[i]->addr == change_point[i]->pbios->addr) &&
           (change_point[i - 1]->addr != change_point[i - 1]->pbios->addr))) {
        // 如果当前元素的地址小于前一个元素的地址
        // 或者，如果当前元素的地址与前一个元素的地址相同
        // 并且当前元素的地址等于当前元素的pbios属性的地址
        // 但前一个元素的地址与前一个元素的pbios属性的地址不同
        change_tmp = change_point[i];
        change_point[i] = change_point[i - 1];
        change_point[i - 1] = change_tmp;
        still_changing = 1;   // 设置标志位，表示进行了交换操作
      }
    }
  }

  /* create a new bios memory map, removing overlaps */
  // 创建一个新的BIOS内存映射，移除重叠

  overlap_entries = 0;    // 重叠表中的条目数量
  /* number of entries in the overlap table */  
  new_bios_entry = 0;     // 创建新BIOS映射条目的索引
  /* index for creating new bios map entries */  
  last_type = 0;          // 从未定义的内存类型开始
  /* start with undefined memory type */  
  last_addr = 0;          // 从0作为最后的起始地址开始
  /* start with 0 as last starting address */  
  /* loop through change-points, determining affect on the new bios map */
  for (chgidx = 0; chgidx < chg_nr; chgidx++) { // 循环遍历变化点，确定对新 BIOS 映射的影响
    /* keep track of all overlapping bios entries */
    // 跟踪所有重叠的BIOS条目
    if (change_point[chgidx]->addr == change_point[chgidx]->pbios->addr) {
      /* add map entry to overlap list (> 1 entry implies an overlap) */
      // 将映射条目添加到重叠列表中（> 1条目意味着有重叠）
      overlap_list[overlap_entries++] = change_point[chgidx]->pbios;
    } else {
      /* remove entry from list (order independent, so swap with last) */
      // 从列表中移除条目（顺序无关，所以与最后一个交换）
      for (i = 0; i < overlap_entries; i++) {
        if (overlap_list[i] == change_point[chgidx]->pbios)
          overlap_list[i] = overlap_list[overlap_entries - 1];
      }
      overlap_entries--;
    }
    /* if there are overlapping entries, decide which "type" to use */
    /* (larger value takes precedence -- 1=usable, 2,3,4,4+=unusable) */
    // 如果有重叠的条目，决定使用哪种“类型”
    // （较大的值优先——1=可用，2,3,4,4+=不可用）
    current_type = 0;
    for (i = 0; i < overlap_entries; i++) // 代码确定重叠段中的“类型”。类型值较大的段优先
      if (overlap_list[i]->type > current_type)
        current_type = overlap_list[i]->type;
    /* continue building up new bios map based on this information */
    // 根据这些信息继续构建新的BIOS映射
    if (current_type != last_type) {  // 如果当前类型与前一个类型不同，那么需要在新的 BIOS 映射中添加或结束一个条目
      if (last_type != 0) { 
        new_bios[new_bios_entry].size = change_point[chgidx]->addr - last_addr;
        /* move forward only if the new size was non-zero */
        // 仅当新大小不为零时才向前移动
        if (new_bios[new_bios_entry].size != 0)
          if (++new_bios_entry >= E820MAX)  // 没有更多空间用于新的 bios 条目
            break; /* no more space left for new bios entries */
      }
      if (current_type != 0) {    // 如果当前类型不为0，那么在新的 BIOS 映射中开始一个新条目
        new_bios[new_bios_entry].addr = change_point[chgidx]->addr;
        new_bios[new_bios_entry].type = current_type;
        last_addr = change_point[chgidx]->addr;
      }
      last_type = current_type; // 更新最后一种类型为当前类型，为下一次迭代做准备
    }
  }
  // 保留新BIOS条目的数量
  new_nr = new_bios_entry; /* retain count for new bios entries */

  /* copy new bios mapping into original location */
  // 将新的BIOS映射复制到原始位置
  memcpy(biosmap, new_bios, new_nr * sizeof(struct e820entry));
  *pnr_map = new_nr;

  // printk("sanitize_e820_map end\n");

  return 0;
}

static void __init add_memory_region(unsigned long long start,
                                     unsigned long long size, int type) {
  // printk("add_memory_region start\n");

  int x = e820.nr_map;  // 获取当前内存区域的数量
  // 检查内存区域数量是否已达到最大值
  if (x == E820MAX) {  // 如果是最大值，则打印错误信息并返回
    printk("Ooops! Too many entries in the memory map!\n");
    return;
  }
  // 添加新的内存区域到内存映射表中
  e820.map[x].addr = start;  // 设置内存区域的起始地址
  e820.map[x].size = size;   // 设置内存区域的类型
  e820.map[x].type = type;   // 增加内存区域的数量
  e820.nr_map++;

  // printk("add_memory_region end\n");
}

static int __init copy_e820_map(struct e820entry *biosmap, int nr_map) {
  // 用于处理BIOS提供的E820内存映射表，帮助操作系统知道哪些内存区域可以用于什么用途（比如RAM，保留区域等）
  // 判断E820表中的条目数量是否小于2。如果是，函数返回-1并退出。
  // 次函数主要是拷贝和修正从BIOS提供的E820内存映射表，并用add_memory_region函数（未在代码中给出）来添加这些区域到系统的内存映射中。

  // printk("copy_e820_map start\n");

  // 因为有效的内存映射至少需要两个区域（通常是可用RAM和保留区）。
  /* Only one memory region (or negative)? Ignore it */
  if (nr_map < 2) return -1;

  do {
    // 从当前条目（biosmap指针指向的条目）中提取起始地址、大小、结束地址和类型。
    unsigned long long start = biosmap->addr;
    unsigned long long size = biosmap->size;
    unsigned long long end = start + size;
    unsigned long type = biosmap->type;

    /* Overflow in 64 bits? Ignore the memory map. */
    if (start > end) return -1;

    /*
     * Some BIOSes claim RAM in the 640k - 1M region.
     * Not right. Fix it up.
     */
    if (type ==
        E820_RAM) {  // 某些BIOS错误地标记640K-1M的内存区域为可用RAM。这里进行修正。
      if (start < 0x100000ULL &&
          end > 0xA0000ULL) {  // 检查当前内存区域是否在640K-1M范围内。
        if (start < 0xA0000ULL)  // 如果起始地址在640K以下，将其调整为640K。
          add_memory_region(start, 0xA0000ULL - start, type);
        if (end <= 0x100000ULL)  // 如果结束地址在1M以下，跳过此次循环。
          continue;
        start = 0x100000ULL;  // 重新调整起始地址和大小。
        size = end - start;
      }
    }
    add_memory_region(start, size, type);  // 添加内存区域到内存映射中。
  } while (biosmap++, --nr_map);

  // printk("copy_e820_map end\n");

  return 0;
}

static void __init print_memory_map(char *who) {
  int i;

  for (i = 0; i < e820.nr_map; i++) {
    printk(" %s: %016Lx - %016Lx ", who, e820.map[i].addr,
           e820.map[i].addr + e820.map[i].size);
    switch (e820.map[i].type) {
      case E820_RAM:
        printk("(usable)\n");
        break;
      case E820_RESERVED:
        printk("(reserved)\n");
        break;
      case E820_ACPI:
        printk("(ACPI data)\n");
        break;
      case E820_NVS:
        printk("(ACPI NVS)\n");
        break;
      default:
        printk("type %lu\n", e820.map[i].type);
        break;
    }
  }
}

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT) // 求页面号并向上对齐（+4095并右移12位）
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT) // 求页面号，向下对齐（右移12位）
#define PFN_PHYS(x)	((x) << PAGE_SHIFT) // 左移12位

/*
 * Reserved space for vmalloc and iomap - defined in asm/page.h
 */
#define MAXMEM_PFN	PFN_DOWN(MAXMEM)
#define MAX_NONPAE_PFN	(1 << 20)

static void __init find_max_pfn(void)
{
	// 找到系统中的最大物理帧号（max_pfn）
	int i;

	max_pfn = 0;		// 初始化 max_pfn 为 0，max_pfn 用于存储最大的物理帧号
	for (i = 0; i < e820.nr_map; i++) {
		unsigned long start, end;	// 定义两个无符号长整型变量，用于存储内存区块的起始和结束帧号
		/* RAM? */
		if (e820.map[i].type != E820_RAM)	// 检查 e820 映射表中当前项的类型是否为 RAM，如果不是，则跳过此项
			continue;
		// 计算当前内存区块的起始帧号，PFN_UP 是一个宏，用于将地址向上取整到最近的页边界
		start = PFN_UP(e820.map[i].addr);
		// 计算当前内存区块的结束帧号，PFN_DOWN 是一个宏，用于将地址向下取整到最近的页边界
		end = PFN_DOWN(e820.map[i].addr + e820.map[i].size);
		if (start >= end)	// 检查计算出的起始帧号是否大于等于结束帧号，如果是，则跳过此项
			continue;
		if (end > max_pfn)	// 如果计算出的结束帧号大于当前的 max_pfn，则更新 max_pfn
			max_pfn = end;
	}
}

static unsigned long __init find_max_low_pfn(void)
{
	unsigned long max_low_pfn;

	max_low_pfn = max_pfn;	// 定义一个变量用于存储最大的低端PFN值
	if (max_low_pfn > MAXMEM_PFN) {		// 检查最大的低端PFN值是否超过了预定义的MAXMEM_PFN
		if (highmem_pages == -1)	
			// 如果高端内存页数未设置（初始化为-1），则根据最大PFN和MAXMEM_PFN计算高端内存页数
			highmem_pages = max_pfn - MAXMEM_PFN;
		if (highmem_pages + MAXMEM_PFN < max_pfn)	
			// 如果高端内存页数加上MAXMEM_PFN小于最大PFN，则更新最大PFN为MAXMEM_PFN加上高端内存页数
			max_pfn = MAXMEM_PFN + highmem_pages;
		if (highmem_pages + MAXMEM_PFN > max_pfn) {
			// 如果高端内存页数加上MAXMEM_PFN大于最大PFN，则打印警告信息，并将高端内存页数设置为0
			printk("only %luMB highmem pages available, ignoring highmem size of %uMB.\n", pages_to_mb(max_pfn - MAXMEM_PFN), pages_to_mb(highmem_pages));
			highmem_pages = 0;
		}
		max_low_pfn = MAXMEM_PFN;	// 更新最大的低端PFN值为MAXMEM_PFN
#ifndef CONFIG_HIGHMEM
		/* Maximum memory usable is what is directly addressable */
		// 如果未启用高端内存支持，则打印警告信息，指示将只使用可直接寻址的最大内存量
		printk(KERN_WARNING "Warning only %ldMB will be used.\n",
					MAXMEM>>20);
		if (max_pfn > MAX_NONPAE_PFN)
			// 如果最大PFN超过了非PAE内核的最大PFN，则打印警告信息，建议使用启用PAE的内核
			printk(KERN_WARNING "Use a PAE enabled kernel.\n");
		else
			printk(KERN_WARNING "Use a HIGHMEM enabled kernel.\n");
#else /* !CONFIG_HIGHMEM */
#ifndef CONFIG_X86_PAE
		if (max_pfn > MAX_NONPAE_PFN) {
			// 如果最大PFN超过了非PAE内核的最大PFN，则将最大PFN更新为非PAE内核的最大PFN，并打印警告信息
			max_pfn = MAX_NONPAE_PFN;
			printk(KERN_WARNING "Warning only 4GB will be used.\n");
			printk(KERN_WARNING "Use a PAE enabled kernel.\n");
		}
#endif /* !CONFIG_X86_PAE */
#endif /* !CONFIG_HIGHMEM */
	} else { // 如果最大的低端PFN值未超过预定义的MAXMEM_PFN
		if (highmem_pages == -1)	// 如果高端内存页数未设置（初始化为-1），则将其设置为0
			highmem_pages = 0;
#if CONFIG_HIGHMEM	// 如果启用了高端内存支持
		if (highmem_pages >= max_pfn) {
			// 检查高端内存页数是否大于等于最大PFN，如果是，则打印错误信息，并将高端内存页数设置为0
			printk(KERN_ERR "highmem size specified (%uMB) is bigger than pages available (%luMB)!.\n", pages_to_mb(highmem_pages), pages_to_mb(max_pfn));
			highmem_pages = 0;
		}
		if (highmem_pages) {	// 如果高端内存页数大于0
			if (max_low_pfn-highmem_pages < 64*1024*1024/PAGE_SIZE){
				// 如果最大的低端PFN值减去高端内存页数小于64MB（以页面大小计算），则打印错误信息，并将高端内存页数设置为0
				printk(KERN_ERR "highmem size %uMB results in smaller than 64MB lowmem, ignoring it.\n", pages_to_mb(highmem_pages));
				highmem_pages = 0;
			}
			max_low_pfn -= highmem_pages;
		}
#else
		if (highmem_pages)
			printk(KERN_ERR "ignoring highmem size on non-highmem kernel!\n");
#endif
	}

	return max_low_pfn;
}

/*
 * Register fully available low RAM pages with the bootmem allocator.
 */
// 将完全可用的低内存页（即物理地址低于 max_low_pfn 的页）注册给引导内存分配器。
static void __init register_bootmem_low_pages(unsigned long max_low_pfn)
{
	int i;
	// 遍历 e820 结构体数组中的每个内存映射条目。
	for (i = 0; i < e820.nr_map; i++) {
		unsigned long curr_pfn, last_pfn, size;
		/*
		 * Reserve usable low memory
		 */
		// 如果当前条目的类型不是 E820_RAM，表示该内存区域不可用，跳过处理。
		if (e820.map[i].type != E820_RAM)
			continue;
		/*
		 * We are rounding up the start address of usable memory:
		 */
		// 将当前条目的起始地址向上舍入为页帧号（PFN）并存储在 curr_pfn 中。
		curr_pfn = PFN_UP(e820.map[i].addr);
		// 如果 curr_pfn 大于等于 max_low_pfn，表示该内存区域不在低内存范围内，跳过处理。
		if (curr_pfn >= max_low_pfn)
			continue;
		/*
		 * ... and at the end of the usable range downwards:
		 */
		// 将当前条目的结束地址向下舍入为页帧号并存储在 last_pfn 中。
		last_pfn = PFN_DOWN(e820.map[i].addr + e820.map[i].size);

		// 如果 last_pfn 大于 max_low_pfn，将其限制为 max_low_pfn，确保不超过低内存范围。
		if (last_pfn > max_low_pfn)
			last_pfn = max_low_pfn;

		/*
		 * .. finally, did all the rounding and playing
		 * around just make the area go away?
		 */
		// 如果经过舍入处理后，last_pfn 小于等于 curr_pfn，表示该内存区域被舍弃或不存在，跳过处理。
		if (last_pfn <= curr_pfn)
			continue;

		size = last_pfn - curr_pfn;	// 计算该内存区域的大小（以页帧号表示），存储在 size 中。
		// 调用 free_bootmem 函数，将该内存区域标记为可用，以便引导内存分配器可以将其用于动态内存分配。
		free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
	}
}

static unsigned long __init setup_memory() {
  printk("setup_memory start\n");

  unsigned long bootmap_size, start_pfn, max_low_pfn;

  start_pfn = PFN_UP(__pa(&_end));	// 获取可用内存的起始页帧号
  printk("start_pfn = 0x%x\n", start_pfn);

  find_max_pfn();		// 找到系统中物理内存的最大页帧号
  printk("max_pfn = 0x%x\n", max_pfn);

  // 找到低端内存（low memory）的最大页帧号
	max_low_pfn = find_max_low_pfn();	
  printk("max_low_pfn = 0x%x\n", max_low_pfn);

  #ifdef CONFIG_HIGHMEM
	highstart_pfn = highend_pfn = max_pfn;
	if (max_pfn > max_low_pfn) {
		highstart_pfn = max_low_pfn;
	}
	printk(KERN_NOTICE "%ldMB HIGHMEM available.\n",
		pages_to_mb(highend_pfn - highstart_pfn));	 // 打印可用的高端内存（high memory）大小
#endif
	printk(KERN_NOTICE "%ldMB LOWMEM available.\n",
			pages_to_mb(max_low_pfn));		 // 打印可用的低端内存（low memory）大小
	/*
	 * Initialize the boot-time allocator (with low memory only):
	 */
	bootmap_size = init_bootmem(start_pfn, max_low_pfn);	 // 初始化启动时的内存分配器，并返回分配的引导映射（bootmap）大小
  printk("bootmap_size = 0x%x\n", bootmap_size);

  register_bootmem_low_pages(max_low_pfn);	// 注册低端内存（low memory）的页帧
  
  /*
	 * Reserve the bootmem bitmap itself as well. We do this in two
	 * steps (first step was init_bootmem()) because this catches
	 * the (very unlikely) case of us accidentally initializing the
	 * bootmem allocator with an invalid RAM area.
	 */
	// 把内核和 bootmem位图 所占的内存标记为“保留”，HIGH_MEMORY为1MB，即内核开始的地方
	reserve_bootmem(HIGH_MEMORY, (PFN_PHYS(start_pfn) +
			 bootmap_size + PAGE_SIZE-1) - (HIGH_MEMORY));	// 预留引导映射（bootmap）位图所占用的内存空间
  	/*
	 * reserve physical page 0 - it's a special BIOS page on many boxes,
	 * enabling clean reboots, SMP operation, laptop functions.
	 */
	reserve_bootmem(0, PAGE_SIZE);	 // 预留物理页面0，这是许多主板上的特殊BIOS页面，用于实现干净的重新启动、SMP操作和笔记本功能。

#ifdef CONFIG_SMP
	/*
	 * But first pinch a few for the stack/trampoline stuff
	 * FIXME: Don't need the extra page at 4K, but need to fix
	 * trampoline before removing it. (see the GDT stuff)
	 */
	reserve_bootmem(PAGE_SIZE, PAGE_SIZE);	// 预留一些页面用于堆栈/trampoline（用于实现从实模式切换到保护模式）的内容
#endif
#ifdef CONFIG_ACPI_SLEEP
	/*
	 * Reserve low memory region for sleep support.
	 */
	acpi_reserve_bootmem();	// 预留低端内存（low memory）区域以支持睡眠功能
#endif
#ifdef CONFIG_X86_LOCAL_APIC
	/*
	 * Find and reserve possible boot-time SMP configuration.
	 */
	find_smp_config();	// 查找并预留可能的启动时SMP配置
#endif
#ifdef CONFIG_BLK_DEV_INITRD
	if (LOADER_TYPE && INITRD_START) {
		if (INITRD_START + INITRD_SIZE <= (max_low_pfn << PAGE_SHIFT)) {
			reserve_bootmem(INITRD_START, INITRD_SIZE);	// 预留用于initrd的内存空间
			initrd_start =
				INITRD_START ? INITRD_START + PAGE_OFFSET : 0;	// 设置initrd的起始地址
			initrd_end = initrd_start+INITRD_SIZE;		// 设置initrd的结束地址
		}
		else {
			printk(KERN_ERR "initrd extends beyond end of memory "
			    "(0x%08lx > 0x%08lx)\ndisabling initrd\n",
			    INITRD_START + INITRD_SIZE,
			    max_low_pfn << PAGE_SHIFT);
			initrd_start = 0;		// 初始化超出内存范围的情况，禁用initrd
		}
	}
#endif

  printk("setup_memory end\n");

	return max_low_pfn;
}

static void __init setup_memory_region(void) {
  // printk("setup_memory_region start\n");

  show_memory_map();
  biosmap_init();
  sanitize_e820_map(biosmap.map, &biosmap.nr_map);
  if (copy_e820_map(biosmap.map, biosmap.nr_map) < 0) {
    PANIC();
  }

  // 使用 printk 打印内存映射信息的标题字符串，以及调用 print_memory_map
  // 函数打印内存映射的详细信息
  print_memory_map("BIOS-e820");

  // printk("setup_memory_region end\n");
}

void __init setup_arch() {
  // printk("setup_arch start\n");

  unsigned long max_low_pfn;

  setup_memory_region();  // 设置内存区域。

  // 设置内存，并将最大低端页面帧号存储在max_low_pfn中。
	max_low_pfn = setup_memory();

  // printk("setup_arch end\n");
}