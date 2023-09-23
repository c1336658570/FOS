/*
 * =====================================================================================
 *
 *       Filename:  multiboot.h
 *
 *    Description:  Multiboot 结构的定义
 *
 *        Version:  1.0
 *        Created:  2013年10月31日 13时08分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Hurley (LiuHuan), liuhuan1992@gmail.com
 *        Company:  Class 1107 of Computer Science and Technology
 *
 * =====================================================================================
 */
#ifndef INCLUDE_MULTIBOOT_H_
#define INCLUDE_MULTIBOOT_H_

#include <asm/types.h>
/**
 * 参考 Multiboot 规范中的 Machine state
 * 启动后，在32位内核进入点，机器状态如下：
 *   1. CS 指向基地址为 0x00000000，限长为4G – 1的代码段描述符。
 *   2. DS，SS，ES，FS 和 GS 指向基地址为0x00000000，限长为4G –
 * 1的数据段描述符。
 *   3. A20 地址线已经打开。
 *   4. 页机制被禁止。
 *   5. 中断被禁止。
 *   6. EAX = 0x2BADB002，魔数，说明操作系统是由符合 Multiboot 的 boot loader
 * 加载的
 *   7. 系统信息和启动信息块的线性地址保存在 EBX中（相当于一个指针）。
 *      以下即为这个信息块的结构（Multiboot information structure）
 */

typedef struct multiboot_t {
  uint32_t flags;  // Multiboot 的版本信息
  /**
   * 从 BIOS 获知的可用内存
   *
   * mem_lower和mem_upper分别指出了低端和高端内存的大小，单位是KB。
   * 低端内存的首地址是0，高端内存的首地址是1M。
   * 低端内存的最大可能值是640K。
   * 高端内存的最大可能值是最大值减去1M。但并不保证是这个值。
   */
  // BIOS报告的可用内存大小，以KB为单位。mem_lower表示低端内存的大小，mem_upper表示高端内存的大小。
  // flags[0]被置位则出现，mem_*域有效
  uint32_t mem_lower;
  uint32_t mem_upper;

  // flags[1]被置位则出现
  uint32_t
      boot_device;  // 指出引导程序从哪个BIOS磁盘设备加载操作系统映像。如果OS映像不是从一个BIOS磁盘载入的，这个域就决不能出现（第3位必须是0）
  // flag[2],cmdline域有效，并包含要传送给内核的命令行参数的物理地址
  uint32_t cmdline;  // 内核命令行的地址
  // flag[3]  mods域指出了同内核一同载入的有哪些引导模块，以及在哪里能找到它们
  uint32_t mods_count;  // boot 模块列表        引导模块列表的数量和地址
  uint32_t mods_addr;  // 引导模块列表的数量和地址

  /**
   * "flags"字段的 bit-5 设置了
   * ELF 格式内核映像的section头表。
   * 包括ELF 内核的 section header table
   * 在哪里、每项的大小、一共有几项以及作为名字索引的字符串表。
   */
  // 这些字段与ELF格式的内核映像的section头表有关。
  // 它们指定了ELF内核的section header
  // table的地址、大小、项数以及名称索引的字符串表 flag[4] 或 flag[5]
  // 第4位和第5位是互斥的。
  uint32_t num;    // 个数
  uint32_t size;   // 大小
  uint32_t addr;   // 起始地址
  uint32_t shndx;  // ELF格式内核映像的section头表中作为名字索引的字符串表的索引
  // 通过该索引值，可以在ELF格式内核映像的section头表中找到对应的字符串表，进而查找和解析其中的字符串信息

  /**
   * 以下两项指出保存由BIOS提供的内存分布的缓冲区的地址和长度
   * mmap_addr是缓冲区的地址，mmap_length是缓冲区的总大小
   * 缓冲区由一个或者多个下面的大小/结构对 mmap_entry_t 组成
   */
  // flag[6]
  //  GRUB将内存探测的结果按每个分段整理为mmap_entry结构体的数组。
  //  mmap_addr是这个结构体数组的首地址，mmap_length是整个数组的长度
  uint32_t mmap_length;
  uint32_t mmap_addr;

  // 第一个驱动器结构的地址和大小
  // flag[7]
  uint32_t drives_length;  // 指出第一个驱动器结构的物理地址
  uint32_t drives_addr;    // 指出第一个驱动器这个结构的大小
  // flag[8]
  uint32_t config_table;  // ROM 配置表       ROM配置表的地址
  // flag[9]
  uint32_t boot_loader_name;  // boot loader 的名字       引导加载程序的名称
  // flag[10]
  uint32_t apm_table;  // APM 表       APM表的地址
  // 与VBE（VESA BIOS扩展）相关的字段
  uint32_t vbe_control_info;  // flag[11]
  uint32_t vbe_mode_info;
  uint32_t vbe_mode;
  uint32_t vbe_interface_seg;
  uint32_t vbe_interface_off;
  uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_t;

/**
 * size是相关结构的大小，单位是字节，它可能大于最小值20
 * base_addr_low是启动地址的低32位，base_addr_high是高32位，启动地址总共有64位
 * length_low是内存区域大小的低32位，length_high是内存区域大小的高32位，总共是64位
 * type是相应地址区间的类型，1代表可用RAM，所有其它的值代表保留区域
 */

// GRUB将内存探测的结果按每个分段整理为mmap_entry结构体的数组
typedef struct mmap_entry_t {
  // size成员指的是除了size之外的成员的大小
  uint32_t size;  // 留意 size 是不含 size 自身变量的大小 结构体本身的大小
  // 内存区域的起始地址，以64位表示
  uint32_t base_addr_low;
  uint32_t base_addr_high;
  uint32_t length_low;
  uint32_t length_high;
  uint32_t
      type;  // 表示相应地址区间的类型，其中1代表可用RAM，其他值代表保留区域
} __attribute__((packed)) mmap_entry_t;

// 声明全局的 multiboot_t * 指针
// 内核未建立分页机制前暂存的指针
// multiboot_t *mboot_ptr_tmp;

extern multiboot_t *glb_mboot_ptr __attribute__((__section__(".data.init")));

#endif  // INCLUDE_MULTIBOOT_H_
