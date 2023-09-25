#include <linux/bootmem.h>
#include <linux/mmzone.h>

// 初始化一个整型变量numnodes为1，用于表示节点数量。在UMA（Uniform Memory Access）平台上进行初始化。
int numnodes = 1;	/* Initialized for UMA platforms */

 // 定义一个静态的bootmem_data_t类型的变量contig_bootmem_data，表示连续物理内存分配的引导数据。
static bootmem_data_t contig_bootmem_data;
// 定义一个pg_data_t类型的变量contig_page_data，并使用初始化器初始化。
// 初始化中包含一个字段bdata，指向先前定义的contig_bootmem_data变量。
// 该变量用于描述连续物理内存分配的引导数据。
pg_data_t contig_page_data = { bdata: &contig_bootmem_data };