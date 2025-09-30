KERNEL_SRC_DIR = C/src/kernel
MINIKERNEL_SRC_DIR = C/src/miniKernel

KERNEL_OBJ_DIR = C/obj/kernel
MINIKERNEL_OBJ_DIR = C/obj/miniKernel

INCLUDE_DIR = C/include
.PHONY: all
.SILENT:

all: build run  

format: disk.img
	

disk.img: 
	dd if=/dev/zero of=disk.img count=204800 bs=512
	mkfs.fat -F 32 disk.img

	dd if=bin/bootloader.bin of=disk.img conv=notrunc
	dd if=bin/fsinfo.bin of=disk.img seek=1 conv=notrunc
	dd if=bin/real_mode.bin of=disk.img seek=2 conv=notrunc
	dd if=bin/bpb_backup.bin of=disk.img seek=5 conv=notrunc
clean:
	rm -rf bin/*
	rm -rf elf/*
	rm -rf obj/*
	rm -rf lib/*

debug: build
	qemu-system-i386 -hda disk.img -S -s -m 4M -no-shutdown -no-reboot -serial tcp:localhost:4321,server,nowait & gdb -ex "target remote:1234" elf/kernel.elf 
	
run : build
	qemu-system-i386 -m 4M  -cpu 486 -hda disk.img -serial tcp:localhost:4321,server,nowait
build: link disk.img
	dd if=bin/miniKernel.bin of=disk.img seek=9 conv=notrunc
	dd if=bin/kernel.bin of=disk.img seek=15 conv=notrunc

bootloader: ASM/boot/bootloader.asm ASM/boot/real_mode.asm ASM/filesystem/fsinfo.asm ASM/filesystem/bpb_backup.asm
	nasm -f elf32 -g -F dwarf -o obj/bootloader.o ASM/boot/bootloader.asm
	nasm -f elf32 -g -F dwarf -o obj/real_mode.o ASM/boot/real_mode.asm
	nasm -f bin -o bin/fsinfo.bin ASM/filesystem/fsinfo.asm
	nasm -f bin -o bin/bpb_backup.bin ASM/filesystem/bpb_backup.asm
	 

kernel: ASM/miniKernel/miniKernelASM.asm  
	nasm -f elf32 -g -F dwarf -o obj/miniKernelASM.o ASM/miniKernel/miniKernelASM.asm
	nasm -f elf32 -g -F dwarf -o obj/kernelASM.o ASM/kernel/kernelASM.asm
	nasm -f elf32 -g -F dwarf -o obj/syscallHandler.o ASM/kernel/syscallHandler.asm

	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/main.o $(KERNEL_SRC_DIR)/core/main.c -I "$(INCLUDE_DIR)" -w 
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/sysio.o $(KERNEL_SRC_DIR)/io/sysio.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/idt.o $(KERNEL_SRC_DIR)/drivers/idt.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/terminal.o $(KERNEL_SRC_DIR)/drivers/terminal.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/string.o $(KERNEL_SRC_DIR)/API/string.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/mmu.o $(KERNEL_SRC_DIR)/drivers/mmu.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/disk.o $(KERNEL_SRC_DIR)/drivers/disk.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/files.o $(KERNEL_SRC_DIR)/drivers/files.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/math.o $(KERNEL_SRC_DIR)/API/math.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/bsod.o $(KERNEL_SRC_DIR)/io/bsod.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/serial.o $(KERNEL_SRC_DIR)/drivers/serial.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/syscallHandler.o $(KERNEL_SRC_DIR)/core/syscallHandler.c -I "$(INCLUDE_DIR)" -w
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/shell.o $(KERNEL_SRC_DIR)/API/shell.c -I "$(INCLUDE_DIR)" -w
	
	
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/miniKernel.o $(MINIKERNEL_SRC_DIR)/miniKernel.c -I "$(INCLUDE_DIR)" -w  
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/mmuMini.o $(MINIKERNEL_SRC_DIR)/mmuMini.c -I "$(INCLUDE_DIR)" -w   
	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/diskMini.o $(MINIKERNEL_SRC_DIR)/diskMini.c -I "$(INCLUDE_DIR)" -w   

	i386-elf-gcc -c -g -ffreestanding -m32 -o obj/kernelAPI.o $(KERNEL_SRC_DIR)/core/kernelAPI.c -I "$(INCLUDE_DIR)" -w 

link: bootloader kernel
	ar rs lib/libos.a obj/kernelAPI.o 
	
	i386-elf-ld -Ttext 0x7C00 -m elf_i386 -o elf/bootloader.elf obj/bootloader.o 
	i386-elf-ld -Ttext 0x1000 -m elf_i386 -o elf/real_mode.elf obj/real_mode.o 
	i386-elf-ld -Ttext 0x2000 -m elf_i386 -o elf/miniKernel.elf obj/miniKernel.o obj/miniKernelASM.o obj/mmuMini.o obj/diskMini.o
	i386-elf-ld -Ttext 0x100008 -m elf_i386 -o elf/kernel.elf obj/kernelASM.o obj/main.o obj/files.o obj/idt.o obj/math.o obj/terminal.o obj/string.o obj/sysio.o obj/mmu.o obj/disk.o obj/bsod.o obj/serial.o obj/syscallHandler.o obj/shell.o
	
	objcopy -O binary elf/bootloader.elf bin/bootloader.bin
	objcopy -O binary elf/real_mode.elf bin/real_mode.bin
	objcopy -O binary elf/kernel.elf bin/kernel.bin
	objcopy -O binary elf/miniKernel.elf bin/miniKernel.bin
