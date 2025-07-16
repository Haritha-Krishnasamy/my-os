# My OS Project: A Deep Dive

## Introduction

Welcome to the documentation for the "My OS" project! This document aims to provide a comprehensive, beginner-friendly explanation of the entire project, from the boot-up process to the kernel's inner workings. Whether you're a seasoned C++ developer or just starting, this guide will walk you through every line of code and concept.

This project is a minimal 32-bit operating system written in C++ and Assembly. It demonstrates fundamental OS concepts like booting, memory management (GDT), and basic kernel functionality.

### What is an Operating System?

At its core, an **Operating System (OS)** is a software that acts as an intermediary between computer hardware and the user. It manages all the hardware resources (like the CPU, memory, and storage devices) and provides a platform for software applications to run. Every computer, from your smartphone to a massive server, runs an OS.

### Why Build Your Own OS?

Building an OS from scratch is one of the most challenging and rewarding projects a programmer can undertake. It forces you to understand the computer at its lowest levels. You'll learn:
*   How a computer boots up.
*   How the CPU interacts with memory and devices.
*   How to manage memory without the help of `malloc` or `new`.
*   How to handle hardware interrupts.
*   The intricate details of processor architecture.

This project is a "toy" OS, meaning it's not intended for daily use. Its purpose is purely educational.

## Table of Contents

1.  [Project Structure](#project-structure)
2.  [Core Concepts for Beginners](#core-concepts-for-beginners)
3.  [The Booting Process: From Power On to `kernelmain`](#the-booting-process)
    *   [The Role of the Bootloader (`loader.s`)](#the-bootloader)
    *   [The Multiboot Specification](#the-multiboot-specification)
    *   [Setting up the Stack](#setting-up-the-stack)
    *   [Calling Global Constructors](#calling-global-constructors)
    *   [Jumping to the Kernel](#jumping-to-the-kernel)
4.  [The Kernel (`kernel.cpp`)](#the-kernel)
    *   [The `kernelmain` Function](#the-kernelmain-function)
    *   [Writing to the Screen (`printf`)](#writing-to-the-screen)
    *   [Global Descriptor Table (GDT)](#global-descriptor-table-gdt)
5.  [Memory Management: The GDT (`gdt.h`, `gdt.cpp`)](#memory-management-the-gdt)
    *   [What is the GDT?](#what-is-the-gdt)
    *   [The `GlobalDescriptorTable` Class](#the-globaldescriptortable-class)
    *   [The `SegmentDescriptor` Class](#the-segmentdescriptor-class)
    *   [Loading the GDT](#loading-the-gdt)
6.  [The Linker Script (`linker.ld`)](#the-linker-script)
    *   [What is a Linker Script?](#what-is-a-linker-script)
    *   [Key Sections of the Script](#key-sections-of-the-script)
7.  [The Build Process (`Makefile`)](#the-build-process)
    *   [Understanding the `Makefile`](#understanding-the-makefile)
    *   [Compiler and Linker Flags](#compiler-and-linker-flags)
    *   [Building and Running the OS](#building-and-running-the-os)
8.  [Data Types (`types.h`)](#data-types)
9.  [Putting It All Together: A Step-by-Step Walkthrough](#a-step-by-step-walkthrough)
10. [Deeper Dive: Flags and Magic Numbers](#deeper-dive-flags-and-magic-numbers)
11. [Practical Guide: Debugging with GDB](#practical-guide-debugging-with-gdb)
12. [Next Steps: Expanding the OS](#next-steps-expanding-the-os)
13. [Interview Questions & Answers](#interview-questions--answers)

---

## Project Structure

Let's start by understanding the layout of the project files:

*   `.gitignore`:  A file that tells the version control system Git which files and folders to ignore. In this project, it's configured to ignore the `.vscode` directory (for Visual Studio Code settings), compiled object files (`*.o`), and the final kernel binary (`*.bin`). This keeps our repository clean from generated files.
*   `gdt.cpp` & `gdt.h`: These files implement the Global Descriptor Table (GDT). The `.h` file defines the structure and the `.cpp` file contains the implementation. The GDT is a crucial part of memory management in x86 systems, which we'll explore in detail.
*   `kernel.cpp`: This is the heart of our operating system. It contains the main kernel function (`kernelmain`), which is the first C++ function to run.
*   `linker.ld`: The linker script. It's a powerful configuration file that tells the linker exactly how to arrange the different sections of our code and data in the final executable file.
*   `loader.s`: The bootloader, written in Assembly language. This is the very first piece of our own code that runs after the computer's initial boot sequence. Its job is to prepare the system for our C++ kernel.
*   `main.cpp`: This file is not part of the OS itself and is not used in the final kernel. It's likely a leftover from a standard C++ project template. In our `Makefile`, we only compile `kernel.cpp`, `gdt.cpp`, and `loader.s`, so `main.cpp` is never linked.
*   `Makefile`: This file contains instructions for the `make` utility to build our project. It automates the process of compiling the source files and linking them together to create the final `mykernel.bin` file.
*   `types.h`: A header file that defines standard, fixed-size integer types for our kernel. This is crucial for portability and direct hardware manipulation.

---

## Core Concepts for Beginners

Before diving into the code, let's clarify a few fundamental concepts.

### CPU Modes: Real Mode vs. Protected Mode

*   **Real Mode**: When an x86 CPU powers on, it starts in Real Mode. This is a legacy 16-bit mode that maintains backward compatibility with the original Intel 8086 processor. In this mode, you can only address 1MB of memory, and there is no memory protection. The BIOS runs in Real Mode.
*   **Protected Mode**: This is the native 32-bit mode of modern x86 CPUs. It allows access to all available system memory (up to 4GB) and provides features like memory protection, paging, and multitasking. Our OS runs in Protected Mode. The GDT is a key component of Protected Mode.

### Kernel Space vs. User Space

Operating systems divide memory into two distinct areas:
*   **Kernel Space**: This is where the kernel code and data reside. The kernel runs in a privileged mode (often called "ring 0") and has unrestricted access to all hardware.
*   **User Space**: This is where user applications run. Code in User Space runs in a restricted mode ("ring 3") and cannot directly access hardware or other applications' memory. It must ask the kernel for services via **system calls**. Our project only implements a kernel and does not yet have a User Space.

### Memory-Mapped I/O

Instead of using special CPU instructions to talk to hardware, many devices map their control registers to physical memory addresses. To communicate with the device, you simply read from or write to these special memory addresses. This is called Memory-Mapped I/O (MMIO). Our `printf` function uses this technique to write to the screen by writing to the video memory at address `0xb8000`.

---

## The Booting Process: From Power On to `kernelmain`

### The Bootloader (`loader.s`)

The journey of our OS begins with the `loader.s` file. This is our **bootloader**, a small piece of code that runs when the computer starts. Its primary job is to transition the CPU from the 16-bit Real Mode to the 32-bit Protected Mode, set up the environment for our C++ kernel, and then hand over control to it.

```assembly
.set MAGIC, 0x1badb002
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM , -(MAGIC + FLAGS)

.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
```

#### The Multiboot Specification

The first few lines of `loader.s` are dedicated to the **Multiboot specification**. Modern bootloaders like GRUB are very powerful and can load many different kinds of operating systems. So how does GRUB know how to load *our* kernel? It needs a standard, a contract. The Multiboot specification is that contract.

By placing a special "Multiboot header" at the beginning of our kernel file, we tell GRUB: "I am a Multiboot-compliant kernel. You can load me by simply placing my contents into memory."

*   `MAGIC` (`0x1badb002`): This is the magic number that GRUB scans for to identify our kernel.
*   `FLAGS`: These flags tell GRUB what information our kernel requires. `(1<<0 | 1<<1)` means we want GRUB to provide us with memory map information and video mode information.
*   `CHECKSUM`: This is a simple sanity check. `MAGIC + FLAGS + CHECKSUM` must equal zero.

```assembly
.section .text
.extern kernelmain
.extern callConstructors
.global loader

loader:
    mov $kernel_stack, %esp
```

#### Setting up the Stack

The `loader` symbol is the entry point of our bootloader, specified in `linker.ld`. The first thing it does is set up the **stack**.

The stack is a crucial region of memory that works in a Last-In, First-Out (LIFO) manner. It's used for storing local variables, function arguments, and return addresses. The `%esp` register is the **stack pointer**; it always points to the top of the stack. The line `mov $kernel_stack, %esp` initializes the stack by pointing `%esp` to a region of memory we've reserved for it (`kernel_stack`). Without a valid stack, we cannot call any functions.

```assembly
    call callConstructors
```

#### Calling Global Constructors

In C++, if you define a global object like `MyClass my_global_object;`, its constructor must be called before `main` (or in our case, `kernelmain`) is executed. In a normal application, the C runtime handles this. In our freestanding kernel, we have to do it ourselves. The `callConstructors` function (defined in `kernel.cpp`) iterates through a list of global object constructors provided by the linker and calls them.

```assembly
    push %eax
    push %ebx
    call kernelmain
```

#### Jumping to the Kernel

After the environment is ready, it's time to jump to our C++ code.
*   `push %eax`, `push %ebx`: The Multiboot specification requires that the magic number be in the `%eax` register and a pointer to the boot information structure be in the `%ebx` register when the kernel is started. We `push` them onto the stack to pass them as arguments to `kernelmain`.
*   `call kernelmain`: This instruction pushes the current instruction pointer (the return address) onto the stack and then jumps to the `kernelmain` function. This is the moment of transition from the Assembly world to the C++ world.

```assembly
_stop:
    cli
    hlt
    jmp _stop
```

If `kernelmain` ever returns (which it shouldn't in a well-behaved OS), this code will execute. `cli` disables interrupts, and `hlt` halts the CPU. The `jmp _stop` creates an infinite loop, effectively freezing the computer. This is a safety measure to prevent the CPU from executing random memory as code.

---

## The Kernel (`kernel.cpp`)

The `kernel.cpp` file is where the main logic of our OS resides. It's written in C++, but with some limitations since we don't have a standard library.

### The `kernelmain` Function

```cpp
extern "C" void kernelmain(void *multiboot_structure, unsigned int magicnumber)
{
    // First, clear the screen
    clear_screen();

    // Print our message
    char *str = (char *)"Hello World from My Kernel!";
    printf(str);

    GlobalDescriptorTable gdt;

    while (1)
        ;
}
```

The `kernelmain` function is the entry point of our C++ kernel.

*   `extern "C"`: This is very important. C++ compilers perform **name mangling**, where they change the name of a function in the compiled output to support features like function overloading. For example, `my_func(int)` and `my_func(float)` become different symbols. Since our Assembly code is calling `kernelmain` by its simple name, we need to tell the C++ compiler not to mangle it. `extern "C"` does exactly that.
*   `void *multiboot_structure, unsigned int magicnumber`: These are the arguments we pushed onto the stack in `loader.s`. They contain the boot information from GRUB.

Inside `kernelmain`, we first clear the screen, then print a "Hello World" message. After that, we create an instance of our `GlobalDescriptorTable`. Simply creating this object on the stack calls its constructor, which sets up and loads the GDT. Finally, we enter an infinite loop (`while(1);`) to prevent the kernel from exiting.

### Writing to the Screen (`printf`)

```cpp
void printf(char *str)
{
    unsigned short *videomemory = (unsigned short *)0xb8000;
    static int cursor_pos = 0; // Keep track of cursor position

    for (int i = 0; str[i] != ' '; ++i)
    {
        // White text on black background (0x0F = white, 0x00 = black)
        videomemory[cursor_pos] = (0x0F << 8) | str[i];
        cursor_pos++;

        // Wrap around if we exceed screen width (80 characters)
        if (cursor_pos >= 80 * 25)
        {
            cursor_pos = 0;
        }
    }
}
```

The `printf` function is a simplified version of the standard C library function. It writes a string to the screen by directly accessing the **video memory**. In 80x25 text mode, the video memory is a block of 4000 bytes starting at the physical address `0xb8000`.

Each character on the screen is represented by two bytes (a `short`):
*   **Byte 1 (ASCII code)**: The character to display (e.g., 'H' is `0x48`).
*   **Byte 2 (Attribute byte)**: The color of the character and its background. The lower 4 bits are the foreground color, and the upper 4 bits are the background color.

In our code, `(0x0F << 8) | str[i]` combines the attribute byte and the character code. `0x0F` means white foreground (`0xF`) on a black background (`0x0`). We write this 16-bit value directly to the video memory, and the character instantly appears on the screen.

---

## Memory Management: The GDT (`gdt.h`, `gdt.cpp`)

### What is the GDT?

The **Global Descriptor Table (GDT)** is a data structure fundamental to x86 Protected Mode. Before the CPU can access any memory in Protected Mode, it needs to know about the valid memory segments. The GDT defines these segments.

A **segment** is a block of memory. The GDT contains entries called **Segment Descriptors**, where each descriptor defines the properties of one segment:
*   Its **base address**: where the segment starts in physical memory.
*   Its **limit**: the size of the segment.
*   Its **access rights**: what can be done with this segment (e.g., is it executable code? Is it writable data?).

The processor has segment registers (`CS`, `DS`, `SS`, etc.). These registers don't hold addresses directly; instead, they hold a **selector**, which is an index into the GDT. When you try to access memory, the CPU uses the selector from the appropriate segment register to look up the segment's properties in the GDT and then performs the access. This allows the OS to control all memory access.

### The `GlobalDescriptorTable` Class

The `GlobalDescriptorTable` class in `gdt.h` and `gdt.cpp` is a convenient C++ wrapper for creating and managing our GDT.

```cpp
// gdt.h
class GlobalDescriptorTable
{
public:
    class SegmentDescriptor { ... };

    SegmentDescriptor null_SegmentSelector;
    SegmentDescriptor unused_SegmentSelector;
    SegmentDescriptor code_SegmentSelector;
    SegmentDescriptor data_SegmentSelector;

public:
    GlobalDescriptorTable();
    ~GlobalDescriptorTable();

    uint16_t Code_SegmentSelector();
    uint16_t Data_SegmentSelector();
};
```

The `GlobalDescriptorTable` class contains four `SegmentDescriptor` objects. In a simple "flat" memory model like ours, we only need a few segments:
*   `null_SegmentSelector`: The first entry in the GDT must be a null descriptor, as required by the x86 architecture.
*   `unused_SegmentSelector`: An unused descriptor.
*   `code_SegmentSelector`: A descriptor for the code segment. This defines the memory area where our kernel code resides. It will be marked as executable.
*   `data_SegmentSelector`: A descriptor for the data segment. This defines the memory area for our kernel's global variables and stack. It will be marked as writable.

### The `SegmentDescriptor` Class

The `SegmentDescriptor` class represents a single 8-byte entry in the GDT.

```cpp
// gdt.h
class SegmentDescriptor
{
private:
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_hi;
    uint8_t type;
    uint8_t flags_limit_hi;
    uint8_t base_vhi;

public:
    SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
    uint32_t Base();
    uint32_t Limit();

} __attribute__((packed));
```

The layout of a GDT entry is strange and non-contiguous, a relic of the 16-bit to 32-bit transition. The base address and limit are split into multiple fields.

*   `__attribute__((packed))`: This is a GCC-specific extension that is absolutely crucial here. It tells the compiler to pack the members of the struct together without any padding bytes. The CPU expects the GDT entries to have a precise, 8-byte layout. Without `packed`, the compiler might add padding to align the members, which would corrupt the GDT and cause a crash.

The constructor of `SegmentDescriptor` takes a base address, a limit (size), and a type (access flags) and correctly populates these bit-fields.

### Loading the GDT

The constructor of the `GlobalDescriptorTable` class is where the GDT is initialized and loaded into the processor.

```cpp
// gdt.cpp
GlobalDescriptorTable::GlobalDescriptorTable()
    : null_SegmentSelector(0, 0, 0),
      unused_SegmentSelector(0, 0, 0),
      code_SegmentSelector(0, 64 * 1024 * 1024, 0x9A), // Code segment
      data_SegmentSelector(0, 64 * 1024 * 1024, 0x92)  // Data segment
{
    uint32_t i[2];
    i[0] = (uint32_t)this;
    i[1] = sizeof(GlobalDescriptorTable) << 16;

    asm volatile("lgdt (%0)" : : "p"(((uint8_t *)i) + 2));
}
```

First, it initializes the code and data segments. We use a base of `0` and a limit of `64MB` for both. This creates a "flat" memory model where code and data can access the same memory range. The access flags `0x9A` and `0x92` specify the segments as 32-bit, executable-readable (code) and readable-writable (data).

Then, it prepares a 6-byte GDT Descriptor structure for the `lgdt` instruction. This structure contains the size of the GDT and its linear address.

Finally, `asm volatile("lgdt ...")` is used. This is GCC inline assembly.
*   `lgdt`: This is the x86 assembly instruction that **L**oads the **G**lobal **D**escriptor **T**able **R**egister (GDTR). The GDTR is a special CPU register that tells the processor where our GDT is located in memory.
*   `volatile`: This tells the compiler not to optimize this assembly block away.
Once `lgdt` is executed, the CPU knows about our new GDT and will use it for all subsequent memory accesses.

---

## The Linker Script (`linker.ld`)

### What is a Linker Script?

A **linker script** is a configuration file that controls the linking process. After the compiler turns our `.cpp` and `.s` files into object files (`.o`), the **linker** (`ld`) takes over. The linker's job is to combine all these object files into a single executable file.

The linker script tells the linker exactly how to do this:
*   What is the entry point of the program?
*   Where should the different sections of the code and data be placed in the final file?
*   What is the memory address where the program should be loaded?

### Key Sections of the Script

```ld
ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
    . = 0x100000;

    .text :
    {
        KEEP(*(.multiboot))
        *(.text*)
        *(.rodata)
    }

    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

*   `ENTRY(loader)`: This specifies that the `loader` symbol (from `loader.s`) is the entry point of our program. This is the first instruction that will be executed.
*   `OUTPUT_FORMAT(elf32-i386)`: This sets the output format to 32-bit ELF (Executable and Linkable Format), a common format for executables on Unix-like systems.
*   `. = 0x100000;`: This is the location counter. This line sets the starting address of our kernel to `0x100000` (1MB). We don't load at address `0` to avoid conflicts with the BIOS data area and other low-memory structures used during boot.
*   `SECTIONS`: This block defines the layout of our executable.
    *   `.text`: This section contains all the executable code (`*(.text*)`) and read-only data (`*(.rodata)`). We use `KEEP(*(.multiboot))` to ensure the linker doesn't discard our Multiboot header.
    *   `.data`: This section contains initialized data (e.g., `int x = 5;`).
    *   `.bss`: This section contains uninitialized data (e.g., `int y;`). The linker reserves space for this section in the executable, but doesn't store the actual zero bytes. The OS is responsible for zeroing it out when loading the program.

---

## The Build Process (`Makefile`)

### Understanding the `Makefile`

A `Makefile` is a script for the `make` utility that automates the build process. It defines a set of rules, where each rule specifies a **target** (a file to be created), its **dependencies** (files it depends on), and the **commands** to run to create the target. This saves us from having to type out long compiler commands every time we change a file.

### Compiler and Linker Flags

```makefile
GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386
```

These flags are crucial for building a freestanding kernel. We are telling the compiler that it cannot rely on any standard libraries or OS features.

*   `-m32`: Compile for a 32-bit architecture.
*   `-nostdlib`: Do not link against the standard C library (`libc`). We don't have one!
*   `-fno-builtin`: Don't use built-in functions that might rely on the standard library (e.g., `memcpy`). We must provide our own implementations if needed.
*   `-fno-rtti`: Disable Run-Time Type Information (RTTI). RTTI allows identifying C++ object types at runtime, but it requires support from the standard library.
*   `-fno-exceptions`: Disable C++ exceptions (`try`/`catch`). The exception handling mechanism also relies on library support.
*   `-fno-use-cxa-atexit`: We don't have a standard C library, so we can't use the standard `atexit` handler for calling global destructors.
*   `-fno-leading-underscore`: Don't add a leading underscore to symbol names, for better compatibility between C++ and Assembly.

### Building and Running the OS

The `Makefile` provides several convenient targets:

*   `make`: This is the default target. It compiles all source files and links them to build the final `mykernel.bin` file.
*   `make install`: This builds the kernel and then runs it in the QEMU emulator. QEMU is a program that emulates a full computer system, allowing us to test our OS without rebooting our actual machine.
*   `make debug`: This runs the kernel in QEMU with the GDB debugger attached. This is extremely useful for debugging and stepping through our kernel code line by line.
*   `make clean`: This removes all the compiled object files (`.o`) and the final kernel binary (`.bin`), allowing for a fresh build.

---

## Data Types (`types.h`)

The `types.h` file is a simple but important part of our project.

```c
// types.h
#ifndef __TYPES_H
#define __TYPES_H

// Define fixed-size integer types
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

#endif
```

In a normal application, the size of types like `int` or `long` can vary between different compilers and architectures. This is unacceptable in OS development, where we often interact directly with hardware that expects data to have a very specific size. For example, a GDT entry must be exactly 8 bytes.

By defining our own fixed-size types (`uint8_t` for an 8-bit unsigned integer, `uint32_t` for a 32-bit unsigned integer, etc.), we ensure that our data structures have a predictable and consistent memory layout, regardless of the compiler.

---

## A Step-by-Step Walkthrough

Let's trace the entire process from power on to our "Hello World" message.

1.  **Power On**: You press the power button. The computer's power supply stabilizes.
2.  **BIOS/UEFI**: The CPU starts executing code from the **BIOS** or **UEFI** firmware, which is stored on a chip on the motherboard. The BIOS performs a Power-On Self-Test (POST) to check that the hardware (RAM, keyboard, etc.) is working.
3.  **Boot Device Selection**: The BIOS looks for a bootable device (like a hard drive or USB stick) based on its configured boot order.
4.  **Loading GRUB**: The BIOS loads the first sector from the boot device, which contains a bootloader like **GRUB**. The BIOS then transfers execution to GRUB.
5.  **Loading Our Kernel**: GRUB is now in control. It understands filesystems, so it can look for our kernel file (`mykernel.bin`). It finds the Multiboot header, recognizes our kernel, and loads the entire file into memory starting at address `0x100000` (as specified in our linker script).
6.  **Executing Our Bootloader (`loader.s`)**: GRUB jumps to the `loader` symbol in our code, which is the entry point.
7.  **Setting up the Stack**: Our assembly code sets the stack pointer (`%esp`) to point to a valid memory region.
8.  **Calling Constructors**: Our code calls the `callConstructors` function to initialize any global C++ objects.
9.  **Jumping to the Kernel**: The bootloader pushes the Multiboot info onto the stack and calls the `kernelmain` function in `kernel.cpp`.
10. **Kernel Execution**: Our C++ kernel code is now running!
    *   It clears the screen.
    *   It calls `printf` to write "Hello World" directly to the video memory at `0xb8000`.
    *   It creates a `GlobalDescriptorTable` object on the stack. The constructor sets up a new GDT and loads it using the `lgdt` instruction.
    *   The kernel enters an infinite loop, waiting for an interrupt or further instructions (which we haven't implemented yet).

---

## Deeper Dive: Flags and Magic Numbers

Certain numbers and flags in the code look cryptic but are essential. Let's break them down.

### GDT Segment Descriptor Type Flags

In `gdt.cpp`, we use `0x9A` for the code segment and `0x92` for the data segment. These 8-bit values are the "type" field of the segment descriptor and are a bitmask that sets access rights.

Let's decode `0x9A` (Code Segment): In binary, this is `10011010`.
*   `[7]`: Present (P) -> `1`: The segment is present in memory.
*   `[6:5]`: Descriptor Privilege Level (DPL) -> `00`: Ring 0 (highest privilege), which is what we want for our kernel.
*   `[4]`: Descriptor Type (S) -> `1`: This is a Code or Data segment (as opposed to a system segment).
*   `[3]`: Executable (E) -> `1`: This is a code segment.
*   `[2]`: Direction/Conforming (DC) -> `0`: The segment grows up.
*   `[1]`: Readable/Writable (RW) -> `1`: The code segment is readable.
*   `[0]`: Accessed (A) -> `0`: The CPU will set this to 1 when the segment is accessed.

Now let's decode `0x92` (Data Segment): In binary, this is `10010010`.
*   `[7]`: Present (P) -> `1`: The segment is present.
*   `[6:5]`: DPL -> `00`: Ring 0.
*   `[4]`: Descriptor Type (S) -> `1`: Code or Data segment.
*   `[3]`: Executable (E) -> `0`: This is a data segment.
*   `[2]`: Direction/Conforming (DC) -> `0`: The segment grows up.
*   `[1]`: Readable/Writable (RW) -> `1`: The data segment is writable.
*   `[0]`: Accessed (A) -> `0`: The CPU will set this to 1 when accessed.

### Multiboot Flags

In `loader.s`, we set `FLAGS` to `(1<<0 | 1<<1)`. This is `0b00000011`.
*   `Bit 0`: If set, the bootloader must align all boot modules on page boundaries (4KB).
*   `Bit 1`: If set, the bootloader must pass a memory map to the kernel.
By setting these, we are telling GRUB, "Please align modules for me and tell me where the usable RAM is."

---

## Practical Guide: Debugging with GDB

The `Makefile` has a `debug` target that is incredibly powerful. It runs QEMU but pauses execution right before the first instruction, waiting for a debugger to connect.

**Step 1: Start the Debug Server**

In your terminal, run:
```sh
make debug
```
This will launch QEMU, but the window will be black and unresponsive. It is now listening for a GDB connection on TCP port 1234.

**Step 2: Start the GDB Client**

Open a **second terminal window**. You need a GDB that is built to understand our target architecture. If you have a cross-compiler toolchain, this will be `i386-elf-gdb`. Run:

```sh
i386-elf-gdb mykernel.bin
```

This starts GDB and loads the symbols from our kernel binary.

**Step 3: Connect to QEMU**

Inside the GDB prompt, type:

```gdb
target remote :1234
```

You will see GDB connect to the QEMU server. Execution is still paused.

**Step 4: Debug!**

You can now use standard GDB commands:
*   `b kernelmain`: Set a breakpoint at the `kernelmain` function.
*   `c`: Continue execution until the breakpoint is hit.
*   `s`: Step one source line.
*   `n`: Step to the next source line (don't step into functions).
*   `i r`: Inspect the registers (`info registers`).
*   `x/16xw 0xb8000`: Examine 16 words of memory in hex format at the video memory address.

This ability to step through your kernel's C++ and Assembly code is the most effective way to find bugs.

---

## Next Steps: Expanding the OS

This project is a great starting point. Here are some features you could add to learn more about OS development, in approximate order of complexity.

1.  **Interrupt Handling:** The `_stop` loop halts the CPU and disables interrupts (`cli`). The next critical step is to set up an **Interrupt Descriptor Table (IDT)**. This is similar to the GDT but for handling interrupts. Once you have an IDT, you can handle hardware events.

2.  **Keyboard Driver:** With interrupts enabled, you can write a simple keyboard driver. You would create an ISR for the keyboard interrupt (IRQ 1). This ISR would read the scancode from the keyboard port, translate it to an ASCII character, and perhaps print it to the screen.

3.  **Basic Memory Management (Paging):** While we have segmentation via the GDT, all modern OSes use **paging**. This involves setting up a Page Directory and Page Tables to map virtual memory addresses to physical memory addresses. Enabling paging is a major milestone that allows for virtual memory, which is a prerequisite for running user-space programs safely.

4.  **A Kernel Heap:** Implement a simple memory allocator (`kmalloc`) to create a kernel heap. This would allow you to dynamically allocate memory for data structures, which is more flexible than using fixed-size global arrays or the stack.

5.  **User-space Programs:** Once you have paging and a system for loading executable files (like ELF files), you can load and run programs in a lower-privilege mode (Ring 3). This involves setting up GDT entries for user-space code and data, and creating a mechanism for **system calls** so user programs can request services from the kernel.

---

## Interview Questions & Answers

Here are some common questions related to the concepts in this project.

**Q1: What is the first thing that runs when you turn on a computer?**
**A:** The first code that runs is not the OS, but the BIOS (Basic Input/Output System) or UEFI (Unified Extensible Firmware Interface). This is firmware stored on a chip on the motherboard. Its job is to initialize the hardware (POST) and then hand over control to a bootloader.

**Q2: What is a bootloader, and what is its primary purpose?**
**A:** A bootloader is a small program that runs after the BIOS. Its primary purpose is to load the actual operating system kernel into memory and transfer execution to it. In our project, `loader.s` is our initial boot code, but in a real scenario, a more powerful bootloader like GRUB would load our `mykernel.bin` file first. GRUB then jumps to our `loader.s` code.

**Q3: What is the GDT and why is it important in Protected Mode?**
**A:** The GDT (Global Descriptor Table) is a data structure required by the x86 architecture to operate in Protected Mode. It's essentially a table of "Segment Descriptors." Each descriptor defines a segment of memory, specifying its base address, size, and access privileges (e.g., readable, writable, executable). The CPU uses the GDT to validate every memory access, providing memory protection and preventing programs from interfering with each other or the kernel.

**Q4: What is the purpose of a linker script in OS development?**
**A:** A linker script gives the developer precise control over the layout of the final executable file. This is critical in OS development for several reasons:
*   **Setting the Load Address:** The kernel must be loaded at a specific physical memory address (e.g., `0x100000`) to avoid conflicts with BIOS data.
*   **Controlling Section Placement:** Specific code or data, like the Multiboot header, must be placed at the very beginning of the file.
*   **Defining Symbols:** It can define symbols that the code can use, like the start or end of the kernel image, which is useful for memory management.

**Q5: Why do you need to disable C++ features like exceptions and RTTI when writing a kernel?**
**A:** These features rely on a hidden runtime support library, which is part of the C++ standard library. This support library, in turn, depends on services from an underlying operating system (e.g., for memory allocation to store RTTI data or to unwind the stack for exceptions). Since we are *writing* the operating system, we don't have another OS underneath us to provide these services. Therefore, we must compile our code in a "freestanding" environment and disable any features that require external runtime support.

**Q6: What does `extern "C"` do and why is it used in `kernelmain`?**
**A:** `extern "C"` tells the C++ compiler to use C-style linkage for a function, which means it will not perform "name mangling" on it. C++ mangles function names to support function overloading (having multiple functions with the same name but different parameters). Our bootloader is written in Assembly and looks for the simple, unmangled symbol `kernelmain`. Using `extern "C"` ensures that the C++ compiler produces a symbol with that exact name, allowing the linker to connect the call from the Assembly code to the C++ function.

**Q7: How does `printf` work in your kernel without a standard library?**
**A:** Our `printf` function works by using **memory-mapped I/O**. It bypasses any standard library or OS calls and writes directly to the hardware's video memory buffer, which is located at the physical address `0xb8000` in text mode. By writing character and color attribute bytes to this memory region, the characters appear on the screen.

**Q8: What is the difference between the stack and the heap? Why doesn't our kernel have a heap?**
**A:** The **stack** is a region of memory used for static memory allocation. It stores local variables, function parameters, and return addresses. It's managed automatically by the CPU; memory is "allocated" by moving the stack pointer and "freed" when a function returns. The **heap** is a region of memory for dynamic allocation (e.g., using `malloc` or `new`). It's managed by the programmer or a library. Our kernel doesn't have a heap because a heap requires a memory manager (an allocator) to keep track of free and used blocks. We haven't written a memory allocator yet, so we can only use the stack and global variables.

**Q9: What is the purpose of the `__attribute__((packed))` in the `SegmentDescriptor` struct?**
**A:** `__attribute__((packed))` is a compiler directive (for GCC/Clang) that tells the compiler to use the minimum possible memory for a struct. By default, compilers often add "padding" bytes between struct members to ensure they are aligned to word boundaries (e.g., a 4-byte integer starts on an address divisible by 4). This can make memory access faster. However, hardware structures like the GDT entries have a rigid, specific layout defined by the CPU manual that contains no padding. `__attribute__((packed))` prevents the compiler from adding this padding, ensuring the C++ struct's memory layout exactly matches the hardware's expectation.

**Q10: What are interrupts and why are they important for an OS?**
**A:** An interrupt is a signal to the CPU from hardware or software, indicating an event that needs immediate attention. When an interrupt occurs, the CPU stops what it's doing, saves its current state, and jumps to a special function called an **Interrupt Service Routine (ISR)** to handle the event. Interrupts are fundamental to an OS for many reasons:
*   **I/O Operations:** Hardware devices like keyboards and network cards use interrupts to signal that they have data ready.
*   **Multitasking:** A timer interrupt can be used to periodically switch between different running tasks, creating the illusion of parallelism.
*   **Error Handling:** Events like division by zero or an invalid memory access trigger interrupts (called exceptions or traps), allowing the OS to handle the error gracefully (e.g., by terminating the offending program).
Our current kernel halts by calling `cli` (Clear Interrupt Flag), which disables external hardware interrupts. A real OS would set up an Interrupt Descriptor Table (IDT) to handle them.

**Q11: What is a "flat memory model"?**
**A:** A flat memory model is a memory architecture where the CPU sees memory as a single, contiguous address space. Our GDT sets up a flat model by creating code and data segments that both start at address 0 and span a large portion of the address space (in our case, 64MB). This is the simplest and most common model used by modern operating systems like Windows and Linux. It's much simpler than a segmented model where you have to constantly switch between different memory segments to access different parts of memory.

**Q12: Why is the kernel loaded at 1MB (`0x100000`) and not at `0x0`?**
**A:** The first megabyte of physical memory in an x86 system is a busy place with a lot of history. It's reserved for use by the BIOS and for compatibility with older hardware and software. For example, the Interrupt Vector Table (in Real Mode), BIOS Data Area, and memory-mapped hardware like the VGA buffer all reside in this low memory area. By loading our kernel at 1MB, we avoid overwriting these critical structures and ensure a clean slate for our OS to run on.

**Q13: In `gdt.cpp`, the `Data_SegmentSelector()` and `Code_SegmentSelector()` functions calculate an offset from `this`. What is this offset and why is it a `uint16_t`?**
**A:** The offset being calculated is the **Segment Selector**. After the GDT is loaded, the CPU's segment registers (like `CS` for Code Segment and `DS` for Data Segment) are not loaded with a direct pointer to the segment descriptor. Instead, they are loaded with a 16-bit value called a segment selector. This selector is an index into the GDT. The value is the byte offset of the desired `SegmentDescriptor` from the beginning of the `GlobalDescriptorTable` object. For example, the code segment descriptor is the third entry, so its offset is `2 * sizeof(SegmentDescriptor) = 16` or `0x10`. This selector is what the CPU uses to find the segment information in the GDT.

**Q14: The `loader.s` file sets up a 2MB space for the stack in the `.bss` section. Why is the stack allocated in `.bss`? What would happen if it were in `.data`?**
**A:** The `.bss` section is used for uninitialized data. The linker reserves space for it in the final executable, but it doesn't take up space in the file itself. The operating system (or in our case, the bootloader) is expected to zero-out this memory region when it loads the program. This is efficient for large, zero-initialized variables like our stack. If we put the 2MB stack in the `.data` section (for initialized data), the resulting `mykernel.bin` file would be at least 2MB larger, as it would have to store 2 million zero bytes. This would make the kernel file unnecessarily large and slow to load.

**Q15: The `Makefile` uses `qemu-system-i386`. What is QEMU and why is it essential for OS development?**
**A:** QEMU is a machine emulator and virtualizer. It can emulate a full computer system (CPU, memory, disks, network cards, etc.) in software. For OS development, it is an indispensable tool because:
*   **Safety:** It allows you to test your OS without running it on your physical hardware. A bug in a kernel can easily crash or even damage a real machine.
*   **Speed:** You can boot and reboot your OS inside QEMU in seconds, whereas rebooting a physical machine is much slower.
*   **Debugging:** QEMU can connect to a debugger like GDB, allowing you to do source-level debugging of your kernel. You can set breakpoints, inspect memory, and step through code line-by-line, which is extremely difficult to do on bare metal.

**Q16: Explain the purpose of the `callConstructors` function and the `start_ctors`/`end_ctors` symbols from the linker script.**
**A:** In C++, global objects can have constructors that need to run before `main` (or `kernelmain`). When compiling, the compiler places pointers to these constructor functions in a special section of the object file, often called `.init_array`. The linker script for a standard application would normally handle this. In our freestanding environment, we must do it manually. 
The linker script (`linker.ld`) defines two symbols, `start_ctors` and `end_ctors`, at the beginning and end of this array of constructor pointers. The C++ function `callConstructors` then iterates through this array, from `start_ctors` to `end_ctors`, and calls each function pointer. This ensures all global objects are properly initialized before our kernel's main logic begins.

**Q17: What is the difference between segmentation and paging? This project only uses segmentation. What are the advantages of adding paging?**
**A:** Both are memory management techniques. 
*   **Segmentation** (which we use) divides the physical address space into a few large, variable-sized segments (like a code segment and a data segment). It provides basic memory protection.
*   **Paging** divides the virtual address space into small, fixed-size blocks called *pages* and the physical address space into blocks called *frames*. It maps pages to frames. 
Paging offers significant advantages over segmentation alone:
*   **Virtual Memory:** It allows each process to have its own 4GB virtual address space, independent of other processes and physical memory layout. This is the foundation of modern multitasking.
*   **Fine-grained Protection:** Protection can be set on a per-page basis (e.g., making a single page read-only).
*   **Demand Paging/Swapping:** Pages can be loaded from disk only when needed, and swapped out to disk if physical memory is full. This allows a system to run programs larger than the available physical RAM.
This project would need to implement a page directory and page tables, and then enable the paging bit in a CPU control register (`CR0`) to add paging.
