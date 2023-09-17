#ifndef __I386_DIV64
#define __I386_DIV64

// i386架构提供了divl，64位被除数放到edx:eax，除数放到其他位置，为什么不直接用divl，而是这么复杂的实现64位除法？
// 直接用divl可能会溢出，如果0x0FFFFFFFFFFFFFFF除1,得到32位商（eax）和32位余数（edx），会溢出


// 在32位x86架构上执行64位除法的宏
// __upper 用于存储高32位的余数，__low 用于存储低32位数值，__high 用于存储高32位数值，__mod 用于存储最终的余数结果。
// 使用内联汇编将输入 n（64位数值）拆分为两个 32 位数值：__low（eax寄存器）和 __high（edx寄存器）。
// 将 __high 的值复制到 __upper 变量中。
// 如果 __high 不为0，则计算 __high 除以 base 的商和余数，并将结果分别存储在 __high 和 __upper 中。
// 执行 divl 指令来进行 64 位除法。结果的商存储在 __low 中，余数存储在 __mod 中。
// 使用内联汇编将 __low 和 __high 重新组合成一个 64 位数值，并存储回 n 中。
// 返回余数。


// "A" (n)：A 约束表示一个 64 位的整数，对于 i386 架构来说，它使用edx:eax来表示。这表示将变量n的低32位放入eax，高32位放入edx。
// "rm" (base)：rm 约束表示此操作数可以是一个寄存器或一个内存位置。这里base是除数。

#define do_div(n,base) ({ \
	unsigned long __upper, __low, __high, __mod; \
	asm("":"=a" (__low), "=d" (__high):"A" (n)); \
	__upper = __high; \
	if (__high) { \
		__upper = __high % (base); \
		__high = __high / (base); \
	} \
	asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (base), "0" (__low), "1" (__upper)); \
	asm("":"=A" (n):"a" (__low),"d" (__high)); \
	__mod; \
})

#endif
