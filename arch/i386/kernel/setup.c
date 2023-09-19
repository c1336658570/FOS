#include <linux/init.h>
#include <asm/e820.h>

struct e820map e820;
extern unsigned long empty_zero_page[1024];

#define PARAM	((unsigned char *)empty_zero_page)
#define SCREEN_INFO (*(struct screen_info *) (PARAM+0))
#define EXT_MEM_K (*(unsigned short *) (PARAM+2))
#define ALT_MEM_K (*(unsigned long *) (PARAM+0x1e0))
#define E820_MAP_NR (*(char*) (PARAM+E820NR))
#define E820_MAP    ((struct e820entry *) (PARAM+E820MAP))	// 从PARAM偏移为E820MAP处开始的struct e820entry结构体的指针。
#define APM_BIOS_INFO (*(struct apm_bios_info *) (PARAM+0x40))
#define DRIVE_INFO (*(struct drive_info_struct *) (PARAM+0x80))
#define SYS_DESC_TABLE (*(struct sys_desc_table_struct*)(PARAM+0xa0))
#define MOUNT_ROOT_RDONLY (*(unsigned short *) (PARAM+0x1F2))
#define RAMDISK_FLAGS (*(unsigned short *) (PARAM+0x1F8))
#define ORIG_ROOT_DEV (*(unsigned short *) (PARAM+0x1FC))
#define AUX_DEVICE_INFO (*(unsigned char *) (PARAM+0x1FF))
#define LOADER_TYPE (*(unsigned char *) (PARAM+0x210))
#define KERNEL_START (*(unsigned long *) (PARAM+0x214))
#define INITRD_START (*(unsigned long *) (PARAM+0x218))
#define INITRD_SIZE (*(unsigned long *) (PARAM+0x21c))
#define COMMAND_LINE ((char *) (PARAM+2048))
#define COMMAND_LINE_SIZE 256

#define RAMDISK_IMAGE_START_MASK  	0x07FF
#define RAMDISK_PROMPT_FLAG		0x8000
#define RAMDISK_LOAD_FLAG		0x4000	

#ifdef	CONFIG_VISWS
char visws_board_type = -1;
char visws_board_rev = -1;

#define	PIIX_PM_START		0x0F80

#define	SIO_GPIO_START		0x0FC0

#define	SIO_PM_START		0x0FC8

#define	PMBASE			PIIX_PM_START
#define	GPIREG0			(PMBASE+0x30)
#define	GPIREG(x)		(GPIREG0+((x)/8))
#define	PIIX_GPI_BD_ID1		18
#define	PIIX_GPI_BD_REG		GPIREG(PIIX_GPI_BD_ID1)

#define	PIIX_GPI_BD_SHIFT	(PIIX_GPI_BD_ID1 % 8)

#define	SIO_INDEX	0x2e
#define	SIO_DATA	0x2f

#define	SIO_DEV_SEL	0x7
#define	SIO_DEV_ENB	0x30
#define	SIO_DEV_MSB	0x60
#define	SIO_DEV_LSB	0x61

#define	SIO_GP_DEV	0x7

#define	SIO_GP_BASE	SIO_GPIO_START
#define	SIO_GP_MSB	(SIO_GP_BASE>>8)
#define	SIO_GP_LSB	(SIO_GP_BASE&0xff)

#define	SIO_GP_DATA1	(SIO_GP_BASE+0)

#define	SIO_PM_DEV	0x8

#define	SIO_PM_BASE	SIO_PM_START
#define	SIO_PM_MSB	(SIO_PM_BASE>>8)
#define	SIO_PM_LSB	(SIO_PM_BASE&0xff)
#define	SIO_PM_INDEX	(SIO_PM_BASE+0)
#define	SIO_PM_DATA	(SIO_PM_BASE+1)

#define	SIO_PM_FER2	0x1

#define	SIO_PM_GP_EN	0x80

static void
visws_get_board_type_and_rev(void)
{
	int raw;

	visws_board_type = (char)(inb_p(PIIX_GPI_BD_REG) & PIIX_GPI_BD_REG)
							 >> PIIX_GPI_BD_SHIFT;
/*
 * Get Board rev.
 * First, we have to initialize the 307 part to allow us access
 * to the GPIO registers.  Let's map them at 0x0fc0 which is right
 * after the PIIX4 PM section.
 */
	outb_p(SIO_DEV_SEL, SIO_INDEX);
	outb_p(SIO_GP_DEV, SIO_DATA);	/* Talk to GPIO regs. */
    
	outb_p(SIO_DEV_MSB, SIO_INDEX);
	outb_p(SIO_GP_MSB, SIO_DATA);	/* MSB of GPIO base address */

	outb_p(SIO_DEV_LSB, SIO_INDEX);
	outb_p(SIO_GP_LSB, SIO_DATA);	/* LSB of GPIO base address */

	outb_p(SIO_DEV_ENB, SIO_INDEX);
	outb_p(1, SIO_DATA);		/* Enable GPIO registers. */
    
/*
 * Now, we have to map the power management section to write
 * a bit which enables access to the GPIO registers.
 * What lunatic came up with this shit?
 */
	outb_p(SIO_DEV_SEL, SIO_INDEX);
	outb_p(SIO_PM_DEV, SIO_DATA);	/* Talk to GPIO regs. */

	outb_p(SIO_DEV_MSB, SIO_INDEX);
	outb_p(SIO_PM_MSB, SIO_DATA);	/* MSB of PM base address */
    
	outb_p(SIO_DEV_LSB, SIO_INDEX);
	outb_p(SIO_PM_LSB, SIO_DATA);	/* LSB of PM base address */

	outb_p(SIO_DEV_ENB, SIO_INDEX);
	outb_p(1, SIO_DATA);		/* Enable PM registers. */
    
/*
 * Now, write the PM register which enables the GPIO registers.
 */
	outb_p(SIO_PM_FER2, SIO_PM_INDEX);
	outb_p(SIO_PM_GP_EN, SIO_PM_DATA);
    
/*
 * Now, initialize the GPIO registers.
 * We want them all to be inputs which is the
 * power on default, so let's leave them alone.
 * So, let's just read the board rev!
 */
	raw = inb_p(SIO_GP_DATA1);
	raw &= 0x7f;	/* 7 bits of valid board revision ID. */

	if (visws_board_type == VISWS_320) {
		if (raw < 0x6) {
			visws_board_rev = 4;
		} else if (raw < 0xc) {
			visws_board_rev = 5;
		} else {
			visws_board_rev = 6;
	
		}
	} else if (visws_board_type == VISWS_540) {
			visws_board_rev = 2;
		} else {
			visws_board_rev = raw;
		}

		printk("Silicon Graphics %s (rev %d)\n",
			visws_board_type == VISWS_320 ? "320" :
			(visws_board_type == VISWS_540 ? "540" :
					"unknown"),
					visws_board_rev);
	}
#endif

// 用于向内核添加内存区域
void __init add_memory_region(unsigned long long start, unsigned long long size, int type)
{
	int x = e820.nr_map;	// 获取当前内存区域的数量
	// 检查内存区域数量是否已达到最大值
	if (x == E820MAX) {		// 如果是最大值，则打印错误信息并返回
	    printk("Ooops! Too many entries in the memory map!\n");
	    return;
	}

	// 添加新的内存区域到内存映射表中
	e820.map[x].addr = start;	// 设置内存区域的起始地址
	e820.map[x].size = size;	// 设置内存区域的类型
	e820.map[x].type = type;	// 增加内存区域的数量
	e820.nr_map++;
}

#define E820_DEBUG	1

static void __init print_memory_map(char *who)
{
	int i;

	for (i = 0; i < e820.nr_map; i++) {
		printk(" %s: %016Lx @ %016Lx ", who,
			e820.map[i].size, e820.map[i].addr);
		switch (e820.map[i].type) {
		case E820_RAM:	printk("(usable)\n");
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
		default:	printk("type %lu\n", e820.map[i].type);
				break;
		}
	}
}

static int __init copy_e820_map(struct e820entry * biosmap, int nr_map)
{
	// 用于处理BIOS提供的E820内存映射表，帮助操作系统知道哪些内存区域可以用于什么用途（比如RAM，保留区域等）
	// 判断E820表中的条目数量是否小于2。如果是，函数返回-1并退出。
	// 次函数主要是拷贝和修正从BIOS提供的E820内存映射表，并用add_memory_region函数（未在代码中给出）来添加这些区域到系统的内存映射中。
	
	// 因为有效的内存映射至少需要两个区域（通常是可用RAM和保留区）。
	if (nr_map < 2)
		return -1;

	do {
		// 从当前条目（biosmap指针指向的条目）中提取起始地址、大小、结束地址和类型。
		unsigned long long start = biosmap->addr;
		unsigned long long size = biosmap->size;
		unsigned long long end = start + size;
		unsigned long type = biosmap->type;

		/* Overflow in 64 bits? Ignore the memory map. */
		if (start > end)
			return -1;

		/*
		 * Some BIOSes claim RAM in the 640k - 1M region.
		 * Not right. Fix it up.
		 */
		if (type == E820_RAM) {	// 某些BIOS错误地标记640K-1M的内存区域为可用RAM。这里进行修正。
			if (start < 0x100000ULL && end > 0xA0000ULL) {	// 检查当前内存区域是否在640K-1M范围内。
				if (start < 0xA0000ULL)	// 如果起始地址在640K以下，将其调整为640K。
					add_memory_region(start, 0xA0000ULL-start, type);
				if (end <= 0x100000ULL)	// 如果结束地址在1M以下，跳过此次循环。
					continue;
				start = 0x100000ULL;		// 重新调整起始地址和大小。
				size = end - start;
			}
		}
		add_memory_region(start, size, type);	// 添加内存区域到内存映射中。
	} while (biosmap++,--nr_map);
	return 0;
}

#define LOWMEMSIZE()	(0x9f000)
//设置系统的物理内存映射
void __init setup_memory_region(void)
{
	char *who = "BIOS-e820";

	/*
	 * Try to copy the BIOS-supplied E820-map.
	 *
	 * Otherwise fake a memory map; one section from 0k->640k,
	 * the next section from 1mb->appropriate_mem_k
	 */
	if (copy_e820_map(E820_MAP, E820_MAP_NR) < 0) {
		unsigned long mem_size;

		/* compare results from other methods and take the greater */
		if (ALT_MEM_K < EXT_MEM_K) {
			mem_size = EXT_MEM_K;
			who = "BIOS-88";
		} else {
			mem_size = ALT_MEM_K;
			who = "BIOS-e801";
		}

		e820.nr_map = 0;
		add_memory_region(0, LOWMEMSIZE(), E820_RAM);
		add_memory_region(HIGH_MEMORY, (mem_size << 10) - HIGH_MEMORY, E820_RAM);
  	}
	printk("BIOS-provided physical RAM map:\n");
	print_memory_map(who);
}

//x86架构的初始化函数，负责设置和初始化系统的内存和其他硬件资源。
void __init setup_arch(char **cmdline_p)
{
	unsigned long bootmap_size;
	unsigned long start_pfn, max_pfn, max_low_pfn;
	int i;
	// 获取VISWS（Visual Workstation）的主板类型和版本
#ifdef CONFIG_VISWS
	visws_get_board_type_and_rev();
#endif
	// 初始化根设备、驱动器信息、屏幕信息、APM BIOS信息等
 	// ROOT_DEV = to_kdev_t(ORIG_ROOT_DEV);
 	// drive_info = DRIVE_INFO;
 	// screen_info = SCREEN_INFO;
	// apm_info.bios = APM_BIOS_INFO;
	// // 如果系统描述表的长度不为0，获取机器ID、子模型ID和BIOS版本
	// if( SYS_DESC_TABLE.length != 0 ) {
	// 	MCA_bus = SYS_DESC_TABLE.table[3] &0x2;
	// 	machine_id = SYS_DESC_TABLE.table[0];
	// 	machine_submodel_id = SYS_DESC_TABLE.table[1];
	// 	BIOS_revision = SYS_DESC_TABLE.table[2];
	// }
	// aux_device_present = AUX_DEVICE_INFO;

	// 如果配置了RAM磁盘，获取其启动图像的位置、提示和加载标志
#ifdef CONFIG_BLK_DEV_RAM
	rd_image_start = RAMDISK_FLAGS & RAMDISK_IMAGE_START_MASK;
	rd_prompt = ((RAMDISK_FLAGS & RAMDISK_PROMPT_FLAG) != 0);
	rd_doload = ((RAMDISK_FLAGS & RAMDISK_LOAD_FLAG) != 0);
#endif
	// 设置内存区域
	setup_memory_region();

// 	// 根据是否设置为只读来调整根挂载标志
// 	if (!MOUNT_ROOT_RDONLY)
// 	root_mountflags &= ~MS_RDONLY;
// 	// 设置内核的代码和数据段的开始和结束地址
// 	init_mm.start_code = (unsigned long) &_text;
// 	init_mm.end_code = (unsigned long) &_etext;
// 	init_mm.end_data = (unsigned long) &_edata;
// 	init_mm.brk = (unsigned long) &_end;

// 	// 设置代码和数据资源的开始和结束地址
// 	code_resource.start = virt_to_bus(&_text);
// 	code_resource.end = virt_to_bus(&_etext)-1;
// 	data_resource.start = virt_to_bus(&_etext);
// 	data_resource.end = virt_to_bus(&_edata)-1;

// 	// 解析命令行中的内存参数
// 	parse_mem_cmdline(cmdline_p);

// 	// 定义一些宏来帮助计算页面帧号和物理地址
// #define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
// #define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
// #define PFN_PHYS(x)	((x) << PAGE_SHIFT)

// /*
//  * 128MB for vmalloc and initrd
//  */
// 	// 为vmalloc和initrd预留128MB的空间
// #define VMALLOC_RESERVE	(unsigned long)(128 << 20)
// #define MAXMEM		(unsigned long)(-PAGE_OFFSET-VMALLOC_RESERVE)
// #define MAXMEM_PFN	PFN_DOWN(MAXMEM)
// #define MAX_NONPAE_PFN	(1 << 20)

// 	/*
// 	 * partially used pages are not usable - thus
// 	 * we are rounding upwards:
// 	 */
// 	// 计算内核结束后的第一个页面帧号
// 	start_pfn = PFN_UP(__pa(&_end));

// 	/*
// 	 * Find the highest page frame number we have available
// 	 */
// 	// 查找可用的最高页面帧号
// 	max_pfn = 0;
// 	for (i = 0; i < e820.nr_map; i++) {
// 		unsigned long start, end;
// 		/* RAM? */
// 		if (e820.map[i].type != E820_RAM)
// 			continue;
// 		start = PFN_UP(e820.map[i].addr);
// 		end = PFN_DOWN(e820.map[i].addr + e820.map[i].size);
// 		if (start >= end)
// 			continue;
// 		if (end > max_pfn)
// 			max_pfn = end;
// 	}

// 	/*
// 	 * Determine low and high memory ranges:
// 	 */
// 	// 确定低内存和高内存的范围
// 	max_low_pfn = max_pfn;
// 	if (max_low_pfn > MAXMEM_PFN) {
// 		max_low_pfn = MAXMEM_PFN;
// #ifndef CONFIG_HIGHMEM
// 		/* Maximum memory usable is what is directly addressable */
// 		printk(KERN_WARNING "Warning only %ldMB will be used.\n",
// 					MAXMEM>>20);
// 		if (max_pfn > MAX_NONPAE_PFN)
// 			printk(KERN_WARNING "Use a PAE enabled kernel.\n");
// 		else
// 			printk(KERN_WARNING "Use a HIGHMEM enabled kernel.\n");
// #else /* !CONFIG_HIGHMEM */
// #ifndef CONFIG_X86_PAE
// 		if (max_pfn > MAX_NONPAE_PFN) {
// 			max_pfn = MAX_NONPAE_PFN;
// 			printk(KERN_WARNING "Warning only 4GB will be used.\n");
// 			printk(KERN_WARNING "Use a PAE enabled kernel.\n");
// 		}
// #endif /* !CONFIG_X86_PAE */
// #endif /* !CONFIG_HIGHMEM */
// 	}

// #ifdef CONFIG_HIGHMEM
// 	highstart_pfn = highend_pfn = max_pfn;
// 	if (max_pfn > MAXMEM_PFN) {
// 		highstart_pfn = MAXMEM_PFN;
// 		printk(KERN_NOTICE "%ldMB HIGHMEM available.\n",
// 			pages_to_mb(highend_pfn - highstart_pfn));
// 	}
// #endif
// 	/*
// 	 * Initialize the boot-time allocator (with low memory only):
// 	 */
// 	// 使用低内存初始化启动时分配器
// 	bootmap_size = init_bootmem(start_pfn, max_low_pfn);

// 	/*
// 	 * Register fully available low RAM pages with the bootmem allocator.
// 	 */
// 	// 使用启动时分配器注册完全可用的低RAM页面
// 	for (i = 0; i < e820.nr_map; i++) {
// 		unsigned long curr_pfn, last_pfn, size;
//  		/*
// 		 * Reserve usable low memory
// 		 */
// 		if (e820.map[i].type != E820_RAM)
// 			continue;
// 		/*
// 		 * We are rounding up the start address of usable memory:
// 		 */
// 		curr_pfn = PFN_UP(e820.map[i].addr);
// 		if (curr_pfn >= max_low_pfn)
// 			continue;
// 		/*
// 		 * ... and at the end of the usable range downwards:
// 		 */
// 		last_pfn = PFN_DOWN(e820.map[i].addr + e820.map[i].size);

// 		if (last_pfn > max_low_pfn)
// 			last_pfn = max_low_pfn;

// 		/*
// 		 * .. finally, did all the rounding and playing
// 		 * around just make the area go away?
// 		 */
// 		if (last_pfn <= curr_pfn)
// 			continue;

// 		size = last_pfn - curr_pfn;
// 		free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
// 	}
// 	/*
// 	 * Reserve the bootmem bitmap itself as well. We do this in two
// 	 * steps (first step was init_bootmem()) because this catches
// 	 * the (very unlikely) case of us accidentally initializing the
// 	 * bootmem allocator with an invalid RAM area.
// 	 */
// 	reserve_bootmem(HIGH_MEMORY, (PFN_PHYS(start_pfn) +
// 			 bootmap_size + PAGE_SIZE-1) - (HIGH_MEMORY));	// 为启动时分配器的位图本身预留内存

// 	/*
// 	 * reserve physical page 0 - it's a special BIOS page on many boxes,
// 	 * enabling clean reboots, SMP operation, laptop functions.
// 	 */
// 	reserve_bootmem(0, PAGE_SIZE);	// 为物理页面0预留内存，因为它在许多机器上是一个特殊的BIOS页面

// #ifdef CONFIG_SMP		// 为SMP配置预留内存
// 	/*
// 	 * But first pinch a few for the stack/trampoline stuff
// 	 * FIXME: Don't need the extra page at 4K, but need to fix
// 	 * trampoline before removing it. (see the GDT stuff)
// 	 */
// 	// 为堆栈/跳板预留一些内存
// 	// FIXME: 在4K处不需要额外的页面，但需要在删除它之前修复跳板（参见GDT部分）
// 	reserve_bootmem(PAGE_SIZE, PAGE_SIZE);	
// 	// 为AP处理器在低内存中分配实模式堆栈
// 	smp_alloc_memory(); /* AP processor realmode stacks in low memory*/
// #endif

// #ifdef CONFIG_X86_IO_APIC
// 	/*
// 	 * Find and reserve possible boot-time SMP configuration:
// 	 */
// 	find_smp_config();		// 查找并预留可能的启动时SMP配置
// #endif
// 	// 初始化分页
// 	paging_init();
// #ifdef CONFIG_X86_IO_APIC
// 	/*
// 	 * get boot-time SMP configuration:
// 	 */
// 	if (smp_found_config)		// 如果找到SMP配置，获取启动时的SMP配置
// 		get_smp_config();
// #endif
// #ifdef CONFIG_X86_LOCAL_APIC
// 	init_apic_mappings();		// 初始化APIC映射
// #endif

// #ifdef CONFIG_BLK_DEV_INITRD
// 	if (LOADER_TYPE && INITRD_START) {	// 如果加载器类型和INITRD_START都存在
// 		if (INITRD_START + INITRD_SIZE <= (max_low_pfn << PAGE_SHIFT)) {	// 如果initrd的大小和起始位置都在低内存范围内
// 			reserve_bootmem(INITRD_START, INITRD_SIZE);	// 预留initrd的内存
// 			initrd_start =
// 				INITRD_START ? INITRD_START + PAGE_OFFSET : 0;
// 			initrd_end = initrd_start+INITRD_SIZE;
// 		}
// 		else {
// 			// 如果initrd超出了内存的末尾，打印警告并禁用initrd
// 			printk("initrd extends beyond end of memory "
// 			    "(0x%08lx > 0x%08lx)\ndisabling initrd\n",
// 			    INITRD_START + INITRD_SIZE,
// 			    max_low_pfn << PAGE_SHIFT);
// 			initrd_start = 0;
// 		}
// 	}
// #endif

// 	/*
// 	 * Request address space for all standard RAM and ROM resources
// 	 * and also for regions reported as reserved by the e820.
// 	 */
// 	probe_roms();	// 探测ROMs
// 	for (i = 0; i < e820.nr_map; i++) {	// 遍历e820映射，为所有标准的RAM和ROM资源以及e820报告为保留的区域请求地址空间
// 		struct resource *res;
// 		if (e820.map[i].addr + e820.map[i].size > 0x100000000ULL)
// 			continue;
// 		res = alloc_bootmem_low(sizeof(struct resource));
// 		switch (e820.map[i].type) {
// 		case E820_RAM:	res->name = "System RAM"; break;
// 		case E820_ACPI:	res->name = "ACPI Tables"; break;
// 		case E820_NVS:	res->name = "ACPI Non-volatile Storage"; break;
// 		default:	res->name = "reserved";
// 		}
// 		res->start = e820.map[i].addr;
// 		res->end = res->start + e820.map[i].size - 1;
// 		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
// 		request_resource(&iomem_resource, res);
// 		if (e820.map[i].type == E820_RAM) {	// 我们不知道哪个RAM区域包含内核数据，所以我们反复尝试并让资源管理器测试它
// 			/*
// 			 *  We dont't know which RAM region contains kernel data,
// 			 *  so we try it repeatedly and let the resource manager
// 			 *  test it.
// 			 */
// 			request_resource(res, &code_resource);
// 			request_resource(res, &data_resource);
// 		}
// 	}
// 	request_resource(&iomem_resource, &vram_resource);	// 请求VRAM资源

// 	/* request I/O space for devices used on all i[345]86 PCs */
// 	for (i = 0; i < STANDARD_IO_RESOURCES; i++)	// 为所有i[345]86 PCs上使用的设备请求I/O空间
// 		request_resource(&ioport_resource, standard_io_resources+i);

// #ifdef CONFIG_VT
// 	// 根据配置选择控制台切换指针
// #if defined(CONFIG_VGA_CONSOLE)
// 	conswitchp = &vga_con;
// #elif defined(CONFIG_DUMMY_CONSOLE)
// 	conswitchp = &dummy_con;
// #endif
// #endif
}