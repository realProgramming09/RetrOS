KERNEL_SRC_DIR = C/src/kernel
MINIKERNEL_SRC_DIR = C/src/miniKernel

KERNEL_OBJ_DIR = C/obj/kernel
MINIKERNEL_OBJ_DIR = C/obj/miniKernel

INCLUDE_DIR = C/include

CC := i386-elf-gcc
CC_FLAGS := -Wall -Wextra -Werror -c -g -ffreestanding -m32 -Wno-sign-compare


LD := i386-elf-ld 
LD_FLAGS := -m elf_i386

OBJCOPY = i386-elf-objcopy
OBJCOPY_FLAGS = -O binary

NASM := nasm
NASM_OBJ_FLAGS := -g -F dwarf -f elf32
NASM_BIN_FLAGS := -f bin

KERNEL_C_FILES := $(wildcard C/src/kernel/core/*.c) \
	 $(wildcard C/src/kernel/API/*.c) \
	 $(wildcard C/src/kernel/drivers/*.c) \
	 $(wildcard C/src/kernel/io/*.c)
KERNEL_ASM_FILES := $(wildcard ASM/kernel/*.asm)
MINIKERNEL_C_FILES := $(wildcard C/src/miniKernel/*.c) 
MINIKERNEL_ASM_FILES := $(wildcard ASM/miniKernel/*.asm)
BOOTLOADER_FILES := $(wildcard ASM/boot/*.asm) $(wildcard ASM/filesystem.asm)

KERNEL_OBJ_FILES := $(patsubst C/src/kernel/%.c, obj/%.o,$(KERNEL_C_FILES))
MINIKERNEL_OBJ_FILES := $(patsubst C/src/miniKernel/%.c, obj/miniKernel/%.o,$(MINIKERNEL_C_FILES)) $(patsubst ASM/miniKernel/%.asm, obj/ASM/%.o, $(MINIKERNEL_ASM_FILES))

 

QEMU := qemu-system-i386
QEMU_FLAGS := -m 4M -cpu 486 -hda disk.img -serial tcp:localhost:4321,server,nowait
QEMU_DEBUG_FLAGS := -hda disk.img -S -s -m 4M -no-shutdown -no-reboot -serial tcp:localhost:4321,server,nowait

.PHONY: all
.SILENT:

all: run  

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
	$(QEMU) $(QEMU_DEBUG_FLAGS) & gdb -ex "target remote:1234" elf/kernel.elf 
	
run : build
	$(QEMU) $(QEMU_FLAGS)
build: copy disk.img
	dd if=bin/miniKernel.bin of=disk.img seek=9 conv=notrunc
	dd if=bin/kernel.bin of=disk.img seek=15 conv=notrunc

bootloader: $(BOOTLOADER_FILES)
	mkdir -p obj obj/bootloader obj/ASM bin
	$(NASM) $(NASM_OBJ_FLAGS) -o obj/bootloader/bootloader.o ASM/boot/bootloader.asm
	$(NASM) $(NASM_OBJ_FLAGS) -o obj/bootloader/real_mode.o ASM/boot/real_mode.asm
	$(NASM) $(NAMS_BIN_FLAGS) -o bin/fsinfo.bin ASM/filesystem/fsinfo.asm
	$(NASM) $(NAMS_BIN_FLAGS) -o bin/bpb_backup.bin ASM/filesystem/bpb_backup.asm
	 

kernel: $(KERNEL_FILES) 
	mkdir -p obj obj/core obj/API obj/drivers obj/io obj/user 

	$(NASM) $(NASM_OBJ_FLAGS) -o obj/ASM/kernelASM.o ASM/kernel/kernelASM.asm
	 
	$(CC) $(CC_FLAGS) -o obj/core/main.o $(KERNEL_SRC_DIR)/core/main.c -I "$(INCLUDE_DIR)" 
	$(CC) $(CC_FLAGS) -o obj/io/sysio.o $(KERNEL_SRC_DIR)/io/sysio.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/drivers/idt.o $(KERNEL_SRC_DIR)/drivers/idt.c -I "$(INCLUDE_DIR)" -Wno-address-of-packed-member
	$(CC) $(CC_FLAGS) -o obj/drivers/terminal.o $(KERNEL_SRC_DIR)/drivers/terminal.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/API/string.o $(KERNEL_SRC_DIR)/API/string.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/drivers/mmu.o $(KERNEL_SRC_DIR)/drivers/mmu.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/drivers/disk.o $(KERNEL_SRC_DIR)/drivers/disk.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/drivers/files.o $(KERNEL_SRC_DIR)/drivers/files.c -I "$(INCLUDE_DIR)"  -Wno-address-of-packed-member
	$(CC) $(CC_FLAGS) -o obj/API/math.o $(KERNEL_SRC_DIR)/API/math.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/io/bsod.o $(KERNEL_SRC_DIR)/io/bsod.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/drivers/serial.o $(KERNEL_SRC_DIR)/drivers/serial.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/core/syscallHandler.o $(KERNEL_SRC_DIR)/core/syscallHandler.c -I "$(INCLUDE_DIR)"  
	$(CC) $(CC_FLAGS) -o obj/API/shell.o $(KERNEL_SRC_DIR)/API/shell.c -I "$(INCLUDE_DIR)" -Wno-unused-parameter 
	$(CC) $(CC_FLAGS) -o obj/user/kernelAPI.o $(KERNEL_SRC_DIR)/user/kernelAPI.c -I "$(INCLUDE_DIR)"  -Wno-discarded-qualifiers
	$(CC) $(CC_FLAGS) -o obj/drivers/timer.o $(KERNEL_SRC_DIR)/drivers/timer.c -I "$(INCLUDE_DIR)"  
miniKernel: 
	mkdir -p obj obj/ASM obj/miniKernel
	$(NASM) $(NASM_OBJ_FLAGS) -o obj/ASM/miniKernelASM.o ASM/miniKernel/miniKernelASM.asm

	$(CC) $(CC_FLAGS) -o obj/miniKernel/miniKernel.o $(MINIKERNEL_SRC_DIR)/miniKernel.c -I "$(INCLUDE_DIR)"    
	$(CC) $(CC_FLAGS) -o obj/miniKernel/mmuMini.o $(MINIKERNEL_SRC_DIR)/mmuMini.c -I "$(INCLUDE_DIR)"     
	$(CC) $(CC_FLAGS) -o obj/miniKernel/diskMini.o $(MINIKERNEL_SRC_DIR)/diskMini.c -I "$(INCLUDE_DIR)"     

	 

link: miniKernel kernel
	ar rs lib/libos.a obj/user/kernelAPI.o 
	
	@echo $(MINIKERNEL_FILES)

	$(LD) -Ttext 0x7C00 $(LD_FLAGS) -o elf/bootloader.elf obj/bootloader/bootloader.o 
	$(LD) -Ttext 0x1000 $(LD_FLAGS) -o elf/real_mode.elf obj/bootloader/real_mode.o 
	$(LD) -Ttext 0x2000 $(LD_FLAGS) -o elf/miniKernel.elf obj/miniKernel/miniKernel.o obj/miniKernel/diskMini.o obj/miniKernel/mmuMini.o obj/ASM/miniKernelASM.o
	$(LD) -Ttext 0x100008 $(LD_FLAGS) -o elf/kernel.elf obj/ASM/kernelASM.o $(KERNEL_OBJ_FILES) 
	
copy: bootloader link
	$(OBJCOPY) $(OBJCOPY_FLAGS) elf/bootloader.elf bin/bootloader.bin
	$(OBJCOPY) $(OBJCOPY_FLAGS) elf/real_mode.elf bin/real_mode.bin
	$(OBJCOPY) $(OBJCOPY_FLAGS) elf/kernel.elf bin/kernel.bin
	$(OBJCOPY) $(OBJCOPY_FLAGS) elf/miniKernel.elf bin/miniKernel.bin
