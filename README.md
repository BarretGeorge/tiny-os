# tiny-os

A modern x86-64 operating system written in C++20 for educational purposes.

## Overview

tiny-os is a monolithic kernel operating system designed to demonstrate modern C++ practices in systems programming. It is part of the "tiny-" series of educational projects, alongside tiny-net, tiny-sql, and tiny-vpn.

**Target Architecture:** x86-64 (64-bit)
**Language:** C++20 (freestanding)
**Bootloader:** Multiboot2 (GRUB2)

## Features

### Current Status

- [x] **Phase 0:** Project structure and build system
- [x] **Phase 1:** Bootloader and basic kernel
- [x] **Phase 2:** Memory management
- [x] **Phase 3:** Interrupt handling
- [x] **Phase 4:** Process and thread management
- [ ] **Phase 5:** File system (FAT32)
- [ ] **Phase 6:** Device drivers and shell
- [ ] **Phase 7:** Documentation and optimization

**Progress: 4/7 phases complete (57%)**

### Implemented Features

**Phase 1: Boot and Basic Kernel** ✅
- ✅ Multiboot2 compliant bootloader
- ✅ x86-64 long mode setup with 4-level paging
- ✅ GDT (Global Descriptor Table) with 5 segments
- ✅ VGA text mode output (80x25, color support, scrolling)
- ✅ Serial port debugging (COM1, 38400 baud)
- ✅ Higher-half kernel mapping (0xFFFFFFFF80000000)

**Phase 2: Memory Management** ✅
- ✅ Physical memory allocator (bitmap-based, 4KB frames)
- ✅ Virtual memory with 4-level paging (PML4→PDPT→PD→PT)
- ✅ Kernel heap allocator (first-fit with free list)
- ✅ C++ new/delete operators
- ✅ Multiboot2 memory map parsing
- ✅ Page fault exception handling

**Phase 3: Interrupt Handling** ✅
- ✅ IDT (Interrupt Descriptor Table) with 256 entries
- ✅ PIC (8259) configuration and remapping
- ✅ Timer driver (PIT at 100 Hz)
- ✅ Exception handlers for all CPU exceptions
- ✅ Detailed error reporting with register dumps
- ✅ Hardware interrupt (IRQ) support

**Phase 4: Process and Thread Management** ✅
- ✅ Process Control Block (PCB) with state management
- ✅ Thread Control Block (TCB) with 16KB stacks
- ✅ Round-robin scheduler with preemptive multitasking
- ✅ Context switching (assembly-optimized)
- ✅ Idle process and thread
- ✅ Demo processes showing concurrent execution
- ⏳ System call interface (planned for later)
- ⏳ fork() implementation (planned for later)

**Phase 5: File System**
- Virtual File System (VFS) layer
- FAT32 file system support
- ATA/IDE disk driver (PIO mode)
- File descriptor table

**Phase 6: Device Drivers and Shell**
- PS/2 keyboard driver
- Interactive shell with commands (ls, cat, echo, ps, mkdir, rm)

**Phase 7: Documentation and Optimization**
- Comprehensive documentation
- Bug fixes and performance optimization
- Test suite

## Quick Start

### Prerequisites

- x86_64-elf cross-compiler (GCC/G++)
- NASM assembler
- CMake 3.20+
- GRUB2 tools (grub-mkrescue)
- QEMU (for testing)

See [BUILD.md](BUILD.md) for detailed installation instructions.

### Building

```bash
./scripts/build.sh
```

This will:
1. Configure the project with CMake
2. Compile the kernel
3. Create a bootable ISO image

### Running in QEMU

```bash
./scripts/run-qemu.sh
```

Press `Ctrl+A` then `X` to exit QEMU.

### Debugging with GDB

```bash
# Terminal 1: Start QEMU with GDB stub
./scripts/debug-qemu.sh

# Terminal 2: Connect GDB
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Project Structure

```
tiny-os/
├── CMakeLists.txt          # Build system configuration
├── README.md               # This file
├── BUILD.md               # Build instructions
├── boot/                  # Bootloader code
│   ├── boot.asm          # Boot assembly
│   └── linker.ld         # Linker script
├── include/tiny_os/       # Header files
│   ├── common/           # Common utilities
│   ├── arch/x86_64/      # x86-64 specific code
│   ├── memory/           # Memory management
│   ├── process/          # Process/thread management
│   ├── fs/               # File system
│   ├── drivers/          # Device drivers
│   └── kernel/           # Core kernel
├── src/                  # Implementation files
├── scripts/              # Build and test scripts
└── docs/                 # Documentation
```

## Architecture

tiny-os uses a **monolithic kernel** architecture where all kernel services run in the same address space. This design is chosen for its simplicity and educational value.

**Key Design Decisions:**
- **x86-64 Long Mode:** Modern 64-bit architecture with cleaner design
- **Higher-Half Kernel:** Kernel mapped at 0xFFFFFFFF80000000
- **Freestanding C++20:** No standard library, custom implementations
- **No Exceptions/RTTI:** Disabled for performance and simplicity

For detailed architecture documentation, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Current Capabilities

tiny-os currently demonstrates a **fully functional multitasking kernel** with:

### Running System
```
tiny-os v0.1.0 - Phase 4
=================================

✅ Multiboot2 boot: OK
✅ 64-bit long mode: OK
✅ GDT setup: OK
✅ VGA text mode: OK
✅ Serial output: OK
✅ C++ runtime: OK
✅ Physical memory: OK
✅ Virtual memory: OK
✅ Heap allocator: OK
✅ Interrupt handling: OK
✅ PIC remapped: OK
✅ Timer (100Hz): OK
✅ Process management: OK
✅ Thread management: OK
✅ Scheduler (Round-Robin): OK
✅ Multitasking: OK
```

### Concurrent Execution
The kernel can run multiple processes simultaneously:
```
[Process 1] Iteration 0
[Process 2] Count 0
[Process 3] Step 0
[Process 1] Iteration 1
[Process 2] Count 1
[Process 3] Step 1
...
```

### Memory Management
- **Physical Memory:** Bitmap allocator tracking 4KB frames
- **Virtual Memory:** 4-level paging with higher-half kernel
- **Heap:** First-fit allocator with 16MB kernel heap

### Interrupt System
- **Timer:** 100 Hz tick rate for scheduling
- **Exceptions:** Full CPU exception handling with detailed dumps
- **IRQs:** Hardware interrupt support via remapped PIC

### Multitasking
- **Scheduler:** Round-Robin with 100ms time slices
- **Context Switch:** Assembly-optimized register save/restore
- **Processes:** Up to 256 concurrent processes
- **Threads:** 16KB kernel stacks per thread

## Development Progress

This project follows a phased implementation approach:

| Phase | Duration | Status |
|-------|----------|--------|
| Phase 0: Project Setup | Week 1 | ✅ **Complete** |
| Phase 1: Boot & Kernel | Week 2-3 | ✅ **Complete** |
| Phase 2: Memory Management | Week 4-6 | ✅ **Complete** |
| Phase 3: Interrupt Handling | Week 7 | ✅ **Complete** |
| Phase 4: Process & Thread Mgmt | Week 8-10 | ✅ **Complete** |
| Phase 5: File System | Week 11-14 | 🔄 Next |
| Phase 6: Drivers & Shell | Week 15-16 | ⏳ Planned |
| Phase 7: Documentation | Week 17-18 | ⏳ Planned |

**Current Milestone:** Preemptive multitasking with 3 concurrent demo processes

## C++20 Features Used

- `constexpr` and `consteval` for compile-time computation
- `concepts` for type constraints
- `std::span` for safe array views (zero overhead)
- Templates and SFINAE for generic programming
- Lambda expressions for callbacks

**Disabled Features:**
- Exceptions (`-fno-exceptions`)
- RTTI (`-fno-rtti`)
- Standard library (freestanding environment)

## Contributing

This is an educational project. Contributions are welcome! Please ensure:
- Code follows C++20 best practices
- Changes are well-documented
- Commits include clear messages
- No breaking changes to existing functionality

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel Software Developer Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Multiboot2 Specification](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
- [Microsoft FAT32 Specification](https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc)

## License

This project is for educational purposes.

## Acknowledgments

Part of the "tiny-" series of educational system programming projects.

---

**🚀 Project Status:** Active Development - **Phase 4 Complete** (Multitasking Enabled!)

**Latest Updates:**
- ✅ Preemptive multitasking with Round-Robin scheduler
- ✅ Context switching between kernel threads
- ✅ Demo processes running concurrently
- 📊 Next: File system implementation (FAT32)