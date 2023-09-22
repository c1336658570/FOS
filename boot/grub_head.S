/* ----------------------------------------------------------------
;
; 	boot.s -- 内核从这里开始
;
;                 这里还有根据 GRUB Multiboot 规范的一些定义
;
 ----------------------------------------------------------------*/
MBOOT_HEADER_MAGIC = 0x1BADB002 	// Multiboot 魔数，由规范决定的

MBOOT_PAGE_ALIGN = 1 << 0    	//0 号位表示所有的引导模块将按页(4KB)边界对齐
MBOOT_MEM_INFO = 1 << 1    	//1 号位通过 Multiboot 信息结构的 mem_* 域包括可用内存的信息
						//(告诉GRUB把内存空间的信息包含在Multiboot信息结构中)
CODE_SELECTOR == 8
//定义我们使用的 Multiboot 的标记
MBOOT_HEADER_FLAGS 	= 	MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO

// 域checksum是一个32位的无符号值，当与其他的magic域(也就是magic和flags)相加时，
// 要求其结果必须是32位的无符号值 0 (即magic + flags + checksum = 0)
MBOOT_CHECKSUM 		= 	- (MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

/* 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头

; Multiboot 头的分布必须如下表所示：
; ----------------------------------------------------------
; 偏移量  类型  域名        备注
;
;   0     u32   magic       必需
;   4     u32   flags       必需
;   8     u32   checksum    必需
;
; 我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
;-----------------------------------------------------------
*/
#include <linux/linkage.h>
.code32  	//所有代码以 32-bit 的方式编译
.section .text.init  	//代码段从这里开始

// 在代码段的起始位置设置符合 Multiboot 规范的标记

.long MBOOT_HEADER_MAGIC   // GRUB 会通过这个魔数判断该映像是否支持
.long MBOOT_HEADER_FLAGS   // GRUB 的一些加载时选项，其详细注释在定义处
.long MBOOT_CHECKSUM       // 检测数值，其含义在定义处

.globl start 		// 内核代码入口，此处提供该声明给 ld 链接器
.globl flush
.extern kern_entry	// 声明内核 C 代码的入口函数
.extern kern_stack
start:
// EAX必须包含魔数0x2BADB002；这个值指出操作系统是被一个符合Multiboot规范的引导程序载入的
// （这样就算是另一种引导程序也可以引导这个操作系统）
// EBX必须包含由引导程序提供的Multiboot信息结构的物理地址（参见引导信息格式）。
// CS必须是一个偏移量位于0到0xFFFFFFFF之间的32位可读/可执行代码段。这里的精确值未定义。
// (CS 指向基地址为 0x00000000，限长为4G – 1的代码段描述符)
// DS ES FS GS SS必须是一个偏移量位于0到0xFFFFFFFF之间的32位可读/可执行代码段。这里的精确值未定义。
// (DS，SS，ES，FS 和 GS 指向基地址为0x00000000，限长为4G–1的数据段描述符。)
// A20 gate必须已经开启。			A20 地址线已经打开。
// CR0 第31位（PG）必须为0。第0位（PE）必须为1。其他位未定义。		页机制被禁止
// EFLAGS
// 第17位（VM）必须为0。第9位（IF）必须为1 。其他位未定义。
// 所有其他的处理器寄存器和标志位未定义。这包括：
// GDTR 尽管段寄存器像上面那样定义了，GDTR也可能是无效的，所以OS映象决不能载入任何段寄存器（即使是载入相同的值也不行！）
// 直到它设定了自己的GDT。
// IDTR			中断被禁止
// OS映象必须在设置完它的IDT之后才能开中断。尽管如此，其他的机器状态应该被引导程序留做正常的工作顺序，
// 也就是同BIOS（或者DOS，如果引导程序是从那里启动的话）初始化的状态一样。换句话说，操作系统应该能够在载入后进行BIOS调用，
// 直到它自己重写BIOS数据结构之前。还有，引导程序必须将PIC设定为正常的BIOS/DOS 状态，尽管它们有可能在进入32位模式时改变它们。

	cli  				//此时还没有设置好保护模式的中断处理，所以必须关闭中断
	movl %ebx, (glb_mboot_ptr)	//将 ebx 中存储的指针存入 glb_mboot_ptr 变量
	movl $0xC0000000, %eax

/*
	leal kern_stack, %esp 
	addl $0x4000, %esp
	subl %eax, %esp
*/

	movl $stack_start, %esp 
	addl $0x2000, %esp

	movl $0, %ebp 			//帧指针修改为 0
	movl $0xB8000, %eax
	movl $0x07560756, (%eax)
	movl $0x07560756, 4(%eax)
	call kern_entry
stop:
	hlt 			 //停机指令，什么也不做，可以降低 CPU 功耗
	jmp stop 		 //到这里结束，关机什么的后面再说

flush:
   ljmp $CODE_SELECTOR, $next
next:
    ret

.section .data.init
ENTRY(stack_start)
 		.fill 4*1024, 1, 0
.previous

/*
.section .data.init
ENTRY(stack_start)
    	.long SYMBOL_NAME(init_task_union)+8192
.previous
*/