# **WELCOME TO RETROS 0.0.1!**
This is an operating system written in a few months by a 16 y/o who likes osdev.

**FEATURES:**
- Monolithic kernel
- No paging, two-level bitmap for up to 256MB of RAM;
- Support for FAT32 
- Support for IDE
- Support for downloading programs via serial (more on that later)
- Two-stage bootloader
- There is a String* API for working in the shell (command parsing) and filesystem (just the software things)
- Syscall library (more on that later too)
- 4 stage compilation ( .c -> .o -> .elf -> .bin) for debugging purposes
- No multitasking

**HOW TO COMPILE:**
- **DEPENDENCIES: dd, make, GNU/Binutils, ar, mkfs, qemu**.
  
  Just type in
-      make
  and everything should work.
  Serial is exposed via a server on **localhost:4321**.
  For debugging, type in
-      make debug
  and GDB will pop up.

**AVAILABLE COMMANDS:**
    Just type in 'help' in the shell and all commands will be shown.

**HOW TO RUN PROGRAMS**
 I provided a .sh file called compileBin.sh that trasforms the .c in a .bin via the same 4-stage compilation process as the OS.
      Example: ./compile.sh yourfile.c -> bin/yourfile.bin
      To send a program to the OS, you must:
 - Inside the RetrOS, type in 'load [program_name]' to wait for connections
 - Outside the RetrOS, use the programsender executable. Example:
 -       ./programsender yourfile.bin
    will send your file to the OS via emulated serial

**HOW TO WORK WITH SERIAL**
Data can be sent and received via sending data to **localhost:4321** via the SLUP protocol. Read the wiki to see how it works

**THE SYSCALL LIBRARY**
     In the lib folder there is a file called libos.a (it will be created at compile-time if it isn't there)
      IMPORTANT: if you wanna use ANY syscall that my OS provides, you have to include C/include/user/kernelAPI.h in your .c
          C/src/core/kernelAPI.c is the file responsible for wrapping syscalls in a more user-friendly way
        
**HOW TO INCLUDE THE SYSCALL LIBRARY**
To use the syscall library in a different directory than the OS's workspace, you have to:
- Copy the include folder and lib/libos.a in your workspace directory
- Compile your .c in a .o file using **i386-elf-gcc**
-  IMPORTANT: you need the **-m32** flag (targets 32bit mode), **-c** (does not link the libc) and **-fPIC** (compiles so that it can be loaded anywhere)
-  Link the .o with libos.a with **i386-elf-ld**. Use the flag **-m elf-i386**, **-L path/to/lib** and **-los**
- Trasform the .elf in a .bin with i386-elf-objcopy. Specify the output format with "-O binary"
Now you can use the programsender executable to load the .bin onto the OS. 

If you wanna debug...good luck. GDB does NOT work with -fPIC. Search the wiki to get better info.


**ADDITIONAL QUIRKS/INFO**
- A sizelog.sh that outputs how much space does the OS consume inside the workspace and inside the VM is provided. It's very useful.
- As of 0.0.1, I can't get emulators other than QEMU running or real hardware.
- As of 0.0.1, everything is in ring 0.
- The OS's files are outside the filesystem; the are located in the 96 sectors reserved before the FAT.
  That means that doing something like "rm -rf /" will only delete YOUR files and make you look like a complete dumbass
