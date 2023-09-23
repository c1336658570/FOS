#include <asm/e820.h>
#include <asm/stdio.h>
#include <linux/debug.h>
#include <linux/init.h>
#include <linux/multiboot.h>
#include <linux/string.h>

struct e820map e820;
struct e820map biosmap;

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

  setup_memory_region();  // 设置内存区域。

  // printk("setup_arch end\n");
}