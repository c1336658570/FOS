//
// Created by ljy on 10/24/21.
//
#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H
#define __init __attribute__ ((__section__ (".text.init")))		// 用于将函数或变量标记为位于".text.init"节的代码s
#define __exit		__attribute__ ((unused, __section__(".text.exit")))	// 用于将函数或变量标记为位于".text.exit"节的代码
#define __initdata	__attribute__ ((__section__ (".data.init")))	// 用于将数据标记为位于".data.init"节的数据
#define __exitdata	__attribute__ ((unused, __section__ (".data.exit")))	// 用于将数据标记为位于".data.exit"节的数据
#define __initsetup	__attribute__ ((unused,__section__ (".setup.init")))	// 用于将函数或变量标记为位于".setup.init"节的代码
#define __init_call	__attribute__ ((unused,__section__ (".initcall.init")))	// 用于将函数或变量标记为位于".initcall.init"节的代码
#define __exit_call	__attribute__ ((unused,__section__ (".exitcall.exit")))	// 用于将函数或变量标记为位于".exitcall.exit"节的代码
/*
 * Used for initialization calls..
 */
typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);
extern initcall_t __initcall_start, __initcall_end;
// 定义宏__initcall，用于将函数标记为初始化调用，并创建一个静态变量__initcall_##fn，将函数fn赋值给它
#define __initcall(fn)								\
	static initcall_t __initcall_##fn __init_call = fn
// 定义宏__exitcall，用于将函数标记为退出调用，并创建一个静态变量__exitcall_##fn，将函数fn赋值给它
#define __exitcall(fn)								\
	static exitcall_t __exitcall_##fn __exit_call = fn
// 定义宏__init_call，用于将函数或变量标记为位于".initcall.init"节的代码
#define __init_call	__attribute__ ((unused,__section__ (".initcall.init")))

void init_all(void);
#endif
