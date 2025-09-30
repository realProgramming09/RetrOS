

i386-elf-gcc -o obj/kernelAPI.o C/src/kernel/core/kernelAPI.c
ar -o libos.a obj/kernelAPI.o

i386-elf-gcc -o obj/$1.o $1 -m32 -g -c -I"C/include"
i386-elf-ld -m elf-i386 -o elf/$1.elf $1.o -los -L lib
i386-elf-objcopy -O binary elf/$1.elf bin/$1.bin