# tiny-os

A modern x86-64 operating system written in C++20 for educational purposes.

## Overview

tiny-os is a monolithic kernel operating system designed to demonstrate modern C++ practices in systems programming. It is part of the "tiny-" series of educational projects, alongside tiny-net, tiny-sql, and tiny-vpn.

**Target Architecture:** x86-64 (64-bit)
**Language:** C++20 (freestanding)
**Bootloader:** Multiboot2 (GRUB2)

## Features

### Current Status

- [x] Project structure and build system
- [ ] Bootloader and basic kernel (Phase 1)
- [ ] Memory management (Phase 2)
- [ ] Interrupt handling (Phase 3)
- [ ] Process and thread management (Phase 4)
- [ ] File system (FAT32) (Phase 5)
- [ ] Device drivers and shell (Phase 6)
- [ ] Documentation and optimization (Phase 7)

### Planned Features

**Phase 1: Boot and Basic Kernel**
- Multiboot2 compliant bootloader
- GDT (Global Descriptor Table) setup
- VGA text mode output
- Serial port debugging

**Phase 2: Memory Management**
- Physical memory allocator (bitmap-based)
- Virtual memory with 4-level paging
- Kernel heap allocator
- C++ new/delete operators

**Phase 3: Interrupt Handling**
- IDT (Interrupt Descriptor Table)
- PIC (8259) configuration
- Timer driver (PIT)
- Exception handlers

**Phase 4: Process and Thread Management**
- Process Control Block (PCB)
- Round-robin scheduler
- Context switching
- System call interface
- fork() implementation

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

## Development Progress

This project follows a phased implementation approach:

| Phase | Duration | Status |
|-------|----------|--------|
| Phase 0: Project Setup | Week 1 | ✅ Complete |
| Phase 1: Boot & Kernel | Week 2-3 | 🔄 In Progress |
| Phase 2: Memory | Week 4-6 | ⏳ Planned |
| Phase 3: Interrupts | Week 7 | ⏳ Planned |
| Phase 4: Processes | Week 8-10 | ⏳ Planned |
| Phase 5: File System | Week 11-14 | ⏳ Planned |
| Phase 6: Drivers & Shell | Week 15-16 | ⏳ Planned |
| Phase 7: Documentation | Week 17-18 | ⏳ Planned |

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

**🚧 Project Status:** Active Development (Phase 0 Complete)