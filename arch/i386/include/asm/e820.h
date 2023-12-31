/*
 * structures and definitions for the int 15, ax=e820 memory map
 * scheme.
 *
 * In a nutshell, arch/i386/boot/setup.S populates a scratch table
 * in the empty_zero_block that contains a list of usable address/size
 * duples.   In arch/i386/kernel/setup.c, this information is
 * transferred into the e820map, and in arch/i386/mm/init.c, that
 * new information is used to mark pages reserved or not.
 *
 */
#ifndef __E820_HEADER
#define __E820_HEADER

#define E820MAX 32 /* number of entries in E820MAP */

#define E820_RAM 1
#define E820_RESERVED 2
#define E820_ACPI 3 /* usable as RAM once ACPI tables have been read */
#define E820_NVS 4

#define HIGH_MEMORY (1024 * 1024)

#ifndef __ASSEMBLY__

struct e820map {             // 整个内存映射表
  int nr_map;                // 存映射表中条目的数量
  struct e820entry {         // 内存映射表中的每个条目
    unsigned long long addr; /* start of memory segment */
    unsigned long long size; /* size of memory segment */
    unsigned long type;      /* type of memory segment */
  } map[E820MAX];
};

extern struct e820map e820;
#endif /*!__ASSEMBLY__*/

#endif /*__E820_HEADER*/
