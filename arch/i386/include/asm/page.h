#ifndef _I386_PAGE_H
#define _I386_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define clear_page(page) memset((void *)(page), 0, PAGE_SIZE)
#define copy_page(to, from) memcpy((void *)(to), (void *)(from), PAGE_SIZE)

/*
 * This handles the memory map.. We could make this a config
 * option, but too many people screw it up, and too few need
 * it.
 *
 * A __PAGE_OFFSET of 0xC0000000 means that the kernel has
 * a virtual address space of one gigabyte, which limits the
 * amount of physical memory you can use to about 950MB.
 *
 * If you want more physical memory than this then see the CONFIG_HIGHMEM4G
 * and CONFIG_HIGHMEM64G options in the kernel configuration.
 */
#define __PAGE_OFFSET (0xC0000000)
#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)
/**
 * P--位0是存在（Present）标志，用于指明表项对地址转换是否有效。P=1表示有效；P=0表示无效。
 * 在页转换过程中，如果说涉及的页目录或页表的表项无效，则会导致一个异常。
 * 如果P=0，那么除表示表项无效外，其余位可供程序自由使用。
 * 例如，操作系统可以使用这些位来保存已存储在磁盘上的页面的序号。
 */
#define PAGE_PRESENT 0x1
#define PAGE_USER 0x4
/**
 * R/W--位1是读/写（Read/Write）标志。如果等于1，表示页面可以被读、写或执行。
 * 如果为0，表示页面只读或可执行。
 * 当处理器运行在超级用户特权级（级别0、1或2）时，则R/W位不起作用。
 * 页目录项中的R/W位对其所映射的所有页面起作用。
 */
#define PAGE_WRITE 0x2

#define __pa(x) ((unsigned long)(x)-PAGE_OFFSET)
#define __va(x) ((void *)((unsigned long)(x) + PAGE_OFFSET))

/*
 * This much address space is reserved for vmalloc() and iomap()
 * as well as fixmap mappings.
 */
#define __VMALLOC_RESERVE \
  (128                    \
   << 20)  // 定义宏__VMALLOC_RESERVE，表示虚拟内存区域保留的大小，这里是128MB
#define VMALLOC_RESERVE \
  ((unsigned long)__VMALLOC_RESERVE)  // 定义宏VMALLOC_RESERVE，表示虚拟内存区域保留的大小，转换为unsigned
                                      // long类型
#define __MAXMEM    \
  (-__PAGE_OFFSET - \
   __VMALLOC_RESERVE)  // 定义宏__MAXMEM，表示系统允许的最大内存大小，通过将__PAGE_OFFSET和__VMALLOC_RESERVE相加并取负数得到
#define MAXMEM \
  ((unsigned long)(-PAGE_OFFSET - VMALLOC_RESERVE))  // 定义宏MAXMEM，表示系统允许的最大内存大小，转换为unsigned
                                                     // long类

typedef struct {
  unsigned long pte_low;
} pte_t;  // 表示页表项（Page Table Entry）
typedef struct {
  unsigned long pmd;
} pmd_t;  // 定义结构体pmd_t，表示页中间目录项（Page Middle Directory Entry）
typedef struct {
  unsigned long pgd;
} pgd_t;  // 表示页全局目录项（Page Global Directory Entry）s
typedef struct {
  unsigned long pgprot;
} pgprot_t;  // 定义结构体pgprot_t，表示页保护位（Page Protection Bits）
#define pte_val(x) ((x).pte_low)  // 获取pte_t结构体中的pte_low成员的值
#define pmd_val(x) ((x).pmd)      // 获取pmd_t结构体中的pmd成员的值
#define pgd_val(x) ((x).pgd)
#define pgprot_val(x) ((x).pgprot)

#define __pte(x) \
  ((pte_t){(x)})  // 用于创建一个pte_t结构体并初始化pte_low成员为x
#define __pmd(x) ((pmd_t){(x)})  // 用于创建一个pgd_t结构体并初始化pgd成员为x
#define __pgd(x) ((pgd_t){(x)})
#define __pgprot(x) ((pgprot_t){(x)})
#define VALID_PAGE(page) \
  ((page - mem_map) <    \
   max_mapnr)  // 用于判断给定的页是否在有效的页范围内，即判断页是否位于mem_map和max_mapnr之间
#define virt_to_page(kaddr) \
  (mem_map +                \
   (__pa(kaddr) >>          \
    PAGE_SHIFT))  // 用于将给定的虚拟地址转换为对应的页结构体指针，通过将虚拟地址的物理地址部分右移PAGE_SHIFT位来计算页索引，再加上mem_map的起始地址来得到页结构体指针

#endif /* _I386_PAGE_H */
