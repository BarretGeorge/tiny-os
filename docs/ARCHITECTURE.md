# tiny-os Architecture

This document describes the architecture and design of tiny-os.

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Boot Process](#boot-process)
4. [Memory Layout](#memory-layout)
5. [Subsystems](#subsystems)
6. [Design Decisions](#design-decisions)

## Overview

tiny-os is a **monolithic kernel** operating system targeting the **x86-64 architecture**. All kernel services (memory management, process scheduling, file systems, device drivers) run in a single address space with full hardware access.

**Key Characteristics:**
- **Architecture:** x86-64 (64-bit long mode)
- **Kernel Type:** Monolithic
- **Language:** C++20 (freestanding)
- **Bootloader:** Multiboot2 compliant (GRUB2)

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    User Space                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │  Shell   │  │  Process │  │  Process │             │
│  │          │  │    A     │  │    B     │             │
│  └──────────┘  └──────────┘  └──────────┘             │
├─────────────────────────────────────────────────────────┤
│                    System Call Interface                │
├─────────────────────────────────────────────────────────┤
│                    Kernel Space                         │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Process Manager                                  │ │
│  │  - Scheduler (Round-Robin)                        │ │
│  │  - Context Switching                              │ │
│  └───────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Memory Manager                                   │ │
│  │  - Physical Allocator (Bitmap)                    │ │
│  │  - Virtual Memory (4-level paging)                │ │
│  │  - Heap Allocator (First-fit)                     │ │
│  └───────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────┐ │
│  │  File System                                      │ │
│  │  - VFS Layer                                      │ │
│  │  - FAT32 Driver                                   │ │
│  └───────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Device Drivers                                   │ │
│  │  - VGA (Text Mode)                                │ │
│  │  - Keyboard (PS/2)                                │ │
│  │  - Timer (PIT)                                    │ │
│  │  - ATA (Disk)                                     │ │
│  └───────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────┐ │
│  │  Interrupt Handling                               │ │
│  │  - IDT (256 entries)                              │ │
│  │  - PIC (8259)                                     │ │
│  │  - Exception Handlers                             │ │
│  └───────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────┤
│                    Hardware Layer                       │
│  CPU | Memory | Disk | Keyboard | Timer | VGA          │
└─────────────────────────────────────────────────────────┘
```

## Boot Process

### Phase 1: BIOS/UEFI

1. BIOS/UEFI performs POST (Power-On Self Test)
2. BIOS loads GRUB2 from boot device
3. GRUB2 loads kernel according to Multiboot2 specification

### Phase 2: Bootloader (boot.asm)

1. **Multiboot2 Header Validation**
   - GRUB verifies Multiboot2 magic number
   - Passes boot information structure to kernel

2. **Long Mode Setup**
   ```
   a. Enable PAE (Physical Address Extension)
      - Set CR4.PAE = 1

   b. Set up initial page tables
      - Identity map first 4MB
      - Map kernel to higher-half (0xFFFFFFFF80000000)

   c. Enable Long Mode
      - Set EFER.LME = 1 (Long Mode Enable)

   d. Enable Paging
      - Load PML4 address to CR3
      - Set CR0.PG = 1

   e. Jump to 64-bit code segment
   ```

3. **Stack Setup**
   - Allocate 16KB kernel stack
   - Set RSP register

4. **Jump to C++ Kernel**
   - Call `kernel_main(uint32_t magic, void* multiboot_info)`

### Phase 3: Kernel Initialization (kernel_main.cpp)

```cpp
kernel_main() {
    // 1. Initialize GDT
    gdt_init();

    // 2. Initialize output (VGA, Serial)
    vga_init();
    serial_init();

    // 3. Initialize memory management
    physical_allocator_init(multiboot_info);
    virtual_allocator_init();
    heap_allocator_init();

    // 4. Initialize interrupt handling
    idt_init();
    pic_init();
    timer_init();

    // 5. Initialize process management
    scheduler_init();

    // 6. Initialize file system
    vfs_init();
    fat32_mount("/dev/sda1", "/");

    // 7. Initialize drivers
    keyboard_init();

    // 8. Start shell
    shell_run();
}
```

## Memory Layout

### Virtual Address Space Layout

```
┌────────────────────────────────────────────────────────┐
│ 0xFFFFFFFFFFFFFFFF                                     │
│ ┌────────────────────────────────────────────────────┐ │
│ │ Kernel Stack                                       │ │
│ │ (16KB)                                             │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ Kernel Heap                                        │ │
│ │ (16MB, expandable)                                 │ │
│ │ 0xFFFFFFFF90000000                                 │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ Kernel Code & Data                                 │ │
│ │ (Higher-half kernel)                               │ │
│ │ 0xFFFFFFFF80000000 ← -2GB                          │ │
│ └────────────────────────────────────────────────────┘ │
│                                                          │
│ ┌────────────────────────────────────────────────────┐ │
│ │ Kernel Direct Mapping                              │ │
│ │ (All physical memory)                              │ │
│ │ 0xFFFF800000000000                                 │ │
│ └────────────────────────────────────────────────────┘ │
│                                                          │
│                   Canonical Hole                        │
│            (Non-canonical addresses)                    │
│                                                          │
│ ┌────────────────────────────────────────────────────┐ │
│ │ User Stack                                         │ │
│ │ (grows down)                                       │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ User Heap                                          │ │
│ │ (grows up)                                         │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ User Code & Data                                   │ │
│ │ (.text, .rodata, .data, .bss)                      │ │
│ ├────────────────────────────────────────────────────┤ │
│ │ Reserved (null pointer dereference protection)     │ │
│ │ 0x0000000000000000                                 │ │
│ └────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

### Physical Memory Layout

```
0x0000000000000000  ┌────────────────────────────────┐
                    │ Real Mode IVT                  │
                    │ (Interrupt Vector Table)       │
0x0000000000000500  ├────────────────────────────────┤
                    │ BIOS Data Area                 │
0x0000000000007C00  ├────────────────────────────────┤
                    │ Bootloader                     │
0x0000000000007E00  ├────────────────────────────────┤
                    │ Free (below 1MB)               │
0x00000000000A0000  ├────────────────────────────────┤
                    │ VGA Memory                     │
0x00000000000C0000  ├────────────────────────────────┤
                    │ BIOS ROM                       │
0x0000000000100000  ├────────────────────────────────┤ ← 1MB
                    │ Kernel (.boot section)         │
                    ├────────────────────────────────┤
                    │ Kernel (.text, .rodata, .data) │
                    ├────────────────────────────────┤
                    │ Kernel (.bss)                  │
kernel_physical_end ├────────────────────────────────┤
                    │ Available RAM                  │
                    │ (Managed by allocators)        │
                    │                                │
                    │                                │
                    └────────────────────────────────┘
```

## Subsystems

### 1. Memory Management

**Physical Allocator (Bitmap)**
- Tracks 4KB frames using bitmap
- 1 bit per frame: 0=free, 1=used
- First-fit allocation algorithm
- O(n) allocation, O(1) free

**Virtual Memory (Paging)**
- 4-level page tables (PML4 → PDPT → PD → PT)
- 4KB page size
- NX bit support (No-Execute)
- On-demand paging for user space

**Heap Allocator (First-Fit)**
- Free list with block headers
- First-fit allocation
- Coalescing of adjacent free blocks

### 2. Process Management

**Process Control Block (PCB)**
```cpp
struct Process {
    uint32_t pid;
    ProcessState state;
    PageTable* page_table;
    VirtualAddr kernel_stack;
    Thread* main_thread;
    FileDescriptorTable* fd_table;
    Process* parent;
};
```

**Scheduler (Round-Robin)**
- 10ms time slices
- FIFO ready queue
- Preemptive multitasking via timer interrupt

**Context Switch**
- Save/restore all GPRs and CPU state
- Switch page tables (CR3)
- Assembly implementation for performance

### 3. File System

**VFS Layer**
- Unified interface for all file systems
- Path resolution with mount points
- File descriptor abstraction

**FAT32 Driver**
- Full read/write support
- Long File Name (LFN) support
- Cluster chain management
- Directory entry caching

### 4. Device Drivers

**VGA (Text Mode)**
- 80x25 character display
- 16 colors
- Hardware cursor

**Keyboard (PS/2)**
- Scancode to ASCII translation
- Shift/Ctrl/Alt modifier support
- Circular input buffer

**Timer (PIT)**
- 100Hz tick rate
- Used for scheduling
- Uptime tracking

**ATA (Disk)**
- PIO mode 0
- LBA28 addressing (up to 128GB)
- Read/write sectors

### 5. Interrupt Handling

**IDT Structure**
- 256 entries
- 0-31: CPU exceptions
- 32-47: Hardware IRQs (PIC)
- 128: System call interrupt

**Exception Handling**
- Page fault handler (INT 14)
- General protection fault (INT 13)
- Debug information on crash

## Design Decisions

### Why Monolithic Kernel?

**Advantages:**
- ✅ Simpler design (no IPC between kernel components)
- ✅ Better performance (no message passing overhead)
- ✅ Easier to debug (everything in one address space)
- ✅ More appropriate for educational purposes

**Disadvantages:**
- ❌ Less modular than microkernel
- ❌ Bugs can crash entire kernel
- ❌ More difficult to isolate components

### Why x86-64 instead of x86 (32-bit)?

**Advantages of x86-64:**
- ✅ Cleaner architecture (no segmentation required)
- ✅ Better toolchain support
- ✅ Simpler paging (4-level vs. PAE's 3-level)
- ✅ More relevant to modern systems

**Disadvantages:**
- ❌ Slightly more complex initial setup (long mode)

### Why FAT32 instead of custom filesystem?

**Advantages:**
- ✅ Well-documented specification
- ✅ Interoperability with other systems
- ✅ Good learning experience
- ✅ Widely used in embedded systems

**Disadvantages:**
- ❌ More complex than a simple custom FS
- ❌ Some legacy quirks (8.3 naming, etc.)

### Why Round-Robin Scheduler?

**Advantages:**
- ✅ Fair scheduling
- ✅ Simple to implement and understand
- ✅ No starvation

**Disadvantages:**
- ❌ Not optimal for real-time tasks
- ❌ No priority support (initially)

**Future Enhancement:** O(1) scheduler with priorities

## Performance Considerations

### Memory Allocator

**Current:** First-fit with linear search
**Future:** Slab allocator for fixed-size objects, buddy allocator for power-of-2

### Scheduler

**Current:** O(n) round-robin
**Future:** O(1) multi-level feedback queue

### File System

**Current:** No caching beyond FAT table
**Future:** Buffer cache, directory entry cache

## Security Considerations

⚠️ **Note:** tiny-os is an educational project and does not implement comprehensive security features.

**Current Security Features:**
- User/kernel mode separation (ring 0 vs ring 3)
- Memory protection via paging
- NX bit support

**Missing (Future Work):**
- ASLR (Address Space Layout Randomization)
- Stack canaries
- Privilege separation
- Capability-based security

## Scalability

**Current Limitations:**
- Single-core only (no SMP support)
- Maximum 4GB RAM addressed by physical allocator (32-bit bitmap)
- LBA28 disk addressing (128GB limit)

**Future Enhancements:**
- SMP support with APIC
- 64-bit physical address support
- LBA48 for larger disks

## References

- [Intel 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Multiboot2 Specification](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)

---

**Document Version:** 1.0
**Last Updated:** 2026-02-08
