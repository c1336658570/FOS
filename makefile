#!Makefile

BUILD_DIR = ./build
C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.S")
S_OBJECTS = $(patsubst %.S, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

#-gdwarf-2:这个选项指定使用DWARF版本2格式的调试信息。DWARF是一种调试信息格式，用于描述程序的源代码和调试相关的信息。
C_FLAGS = -I ./include/ -I ./arch/i386/include -c -fno-builtin -m32 -fno-stack-protector -nostdinc -fno-pic -gdwarf-2
LD_FLAGS = -m elf_i386 -T ./script/kernel.ld -Map ./build/kernel.map -nostdlib
ASM_FLAGS = -f elf -g -F stabs

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

#.c.o和.S.o：模式规则，用于将C源文件和汇编源文件编译为目标文件。
.c.o:
	@echo 编译代码文件 $< ...
	$(CC) $(C_FLAGS) $< -o $@

.S.o:
	@echo 编译汇编文件 $< ...
#	$(ASM) $(ASM_FLAGS) $< -o $@
	$(CC) $(C_FLAGS) $< -o $@

link:
	@echo 链接内核文件...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o kernel.bin

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) kernel.bin

.PHONY:update_image
update_image:
	sudo cp kernel.bin ./hdisk/boot/
	sync;sync;sync

.PHONY:mount_image
mount_image:
	sudo mount -o loop ./hd.img ./hdisk/

.PHONY:umount_image
umount_image:
	sudo umount ./hdisk

.PHONY:qemu
qemu:
	qemu-system-x86_64 -serial stdio -drive file=./hd.img,format=raw,index=0,media=disk -m 512 -device VGA

.PHONY:debug
debug:
	qemu-system-x86_64 -serial stdio -S -s -drive file=./hd.img,format=raw,index=0,media=disk -m 512

gdb:
	qemu-system-x86_64 -serial stdio -S -s -drive file=./hd.img,format=raw,index=0,media=disk -m 512 &
	gdb -x script/gdbinit

#用于模拟x86_64架构的计算机系统		

#-serial stdio: 这个选项将虚拟机的串口重定向到标准输入和输出（stdio）。
#这意味着虚拟机的串口输出将显示在终端上，并且可以通过终端向虚拟机的串口发送输入。

#-S:这个选项告诉QEMU在启动后暂停执行，等待进一步的命令。这样可以在虚拟机启动之前进行调试或其他配置。

#-s:这个选项启用了GDB服务器，可以使用GDB（GNU调试器）来连接到虚拟机的调试接口。

#-drive file=./hd.img,format=raw,index=0,media=disk:这个选项指定了虚拟机的硬盘驱动器配置。
#file=./hd.img表示虚拟机将使用名为"hd.img"的磁盘镜像文件作为硬盘。
#format=raw表示磁盘镜像文件是原始格式的，没有经过任何压缩或编码。
#index=0表示这个驱动器将作为虚拟机的第一个硬盘驱动器。media=disk表示这个驱动器模拟的是一个物理硬盘。

#-m 512:这个选项指定了虚拟机的内存大小。512表示虚拟机的内存大小为512MB。