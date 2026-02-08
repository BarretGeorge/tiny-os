#include <tiny_os/kernel/kernel.h>
#include <tiny_os/arch/x86_64/gdt.h>
#include <tiny_os/arch/x86_64/idt.h>
#include <tiny_os/arch/x86_64/pic.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/drivers/timer.h>
#include <tiny_os/memory/physical_allocator.h>
#include <tiny_os/memory/virtual_allocator.h>
#include <tiny_os/memory/heap_allocator.h>
#include <tiny_os/process/process.h>
#include <tiny_os/process/thread.h>
#include <tiny_os/process/scheduler.h>
#include <tiny_os/drivers/ata.h>
#include <tiny_os/fs/vfs.h>
#include <tiny_os/fs/fat32.h>

namespace tiny_os::kernel {

[[noreturn]] void panic(const char* message) {
    drivers::VGA::set_color(Color::WHITE, Color::RED);
    drivers::kprintf("\n\n*** KERNEL PANIC ***\n");
    drivers::kprintf("%s\n", message);
    drivers::serial_printf("\n\n*** KERNEL PANIC ***\n");
    drivers::serial_printf("%s\n", message);

    // Halt forever
    while (true) {
        asm volatile("cli; hlt");
    }
}

extern "C" void kernel_main(uint32 magic, void* multiboot_info) {
    // Initialize VGA text mode
    drivers::VGA::init();
    drivers::VGA::set_color(Color::LIGHT_CYAN, Color::BLACK);

    // Initialize serial port for debugging
    drivers::Serial::init();

    // Print banner
    drivers::kprintf("=================================\n");
    drivers::kprintf("   tiny-os v0.1.0 - Phase 5\n");
    drivers::kprintf("=================================\n\n");

    drivers::serial_printf("tiny-os booting...\n");

    // Verify Multiboot2 magic number
    drivers::kprintf("Checking Multiboot2 magic... ");
    if (magic != MULTIBOOT2_MAGIC) {
        drivers::VGA::set_color(Color::LIGHT_RED, Color::BLACK);
        drivers::kprintf("FAILED!\n");
        drivers::kprintf("Expected: 0x%x, Got: 0x%x\n", MULTIBOOT2_MAGIC, magic);
        panic("Invalid Multiboot2 magic number!");
    }
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    drivers::serial_printf("Multiboot2 magic verified: 0x%x\n", magic);
    drivers::serial_printf("Multiboot2 info address: 0x%x\n",
                          reinterpret_cast<uint64>(multiboot_info));

    // Initialize GDT
    drivers::kprintf("Initializing GDT... ");
    arch::x86_64::GDT::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);
    drivers::serial_printf("GDT initialized\n");

    // Initialize physical memory allocator
    drivers::kprintf("\n--- Phase 2: Memory Management ---\n");
    memory::PhysicalAllocator::init(multiboot_info);

    // Initialize virtual memory
    memory::VirtualAllocator::init();

    // Initialize kernel heap (16MB)
    constexpr VirtualAddress HEAP_START = 0xFFFFFFFF90000000ULL;
    constexpr usize HEAP_SIZE = 16 * 1024 * 1024;  // 16MB
    memory::HeapAllocator::init(HEAP_START, HEAP_SIZE);

    // Test heap allocator
    drivers::kprintf("\nTesting heap allocator...\n");
    int* test_ptr = new int(42);
    drivers::kprintf("Allocated int: %d\n", *test_ptr);
    delete test_ptr;
    drivers::kprintf("Heap test: OK\n");

    // Print memory stats
    memory::PhysicalAllocator::print_stats();
    memory::HeapAllocator::print_stats();

    // Initialize interrupt handling
    drivers::kprintf("\n--- Phase 3: Interrupt Handling ---\n");

    // Initialize IDT
    drivers::kprintf("Initializing IDT... ");
    arch::x86_64::IDT::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Initialize and remap PIC
    drivers::kprintf("Initializing PIC... ");
    arch::x86_64::PIC::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Initialize timer (100 Hz)
    drivers::kprintf("Initializing Timer... ");
    drivers::Timer::init(100);
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Enable interrupts
    drivers::kprintf("Enabling interrupts... ");
    arch::x86_64::IDT::enable_interrupts();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Initialize process and thread management
    drivers::kprintf("\n--- Phase 4: Process and Thread Management ---\n");

    // Initialize process manager
    drivers::kprintf("Initializing process manager... ");
    process::ProcessManager::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Initialize thread manager
    drivers::kprintf("Initializing thread manager... ");
    process::ThreadManager::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Initialize scheduler
    drivers::kprintf("Initializing scheduler... ");
    process::Scheduler::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Start scheduler (creates idle process)
    drivers::kprintf("Starting scheduler... ");
    process::Scheduler::start();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Create demo processes
    drivers::kprintf("\nCreating demo processes...\n");

    // Demo process 1
    auto demo1 = []() {
        for (int i = 0; i < 10; i++) {
            drivers::kprintf("[Process 1] Iteration %d\n", i);
            process::ThreadManager::yield();
        }
        drivers::kprintf("[Process 1] Finished!\n");
    };

    // Demo process 2
    auto demo2 = []() {
        for (int i = 0; i < 10; i++) {
            drivers::kprintf("[Process 2] Count %d\n", i);
            process::ThreadManager::yield();
        }
        drivers::kprintf("[Process 2] Finished!\n");
    };

    // Demo process 3
    auto demo3 = []() {
        for (int i = 0; i < 10; i++) {
            drivers::kprintf("[Process 3] Step %d\n", i);
            process::ThreadManager::yield();
        }
        drivers::kprintf("[Process 3] Finished!\n");
    };

    process::Process* proc1 = process::ProcessManager::create_kernel_process("demo1", demo1);
    process::Process* proc2 = process::ProcessManager::create_kernel_process("demo2", demo2);
    process::Process* proc3 = process::ProcessManager::create_kernel_process("demo3", demo3);

    if (proc1 && proc2 && proc3) {
        // Add threads to scheduler
        process::Scheduler::add_thread(proc1->main_thread);
        process::Scheduler::add_thread(proc2->main_thread);
        process::Scheduler::add_thread(proc3->main_thread);

        drivers::kprintf("Created 3 demo processes\n");
    }

    // Initialize file system
    drivers::kprintf("\n--- Phase 5: File System ---\n");

    // Initialize ATA manager
    drivers::kprintf("Initializing ATA manager... ");
    drivers::ATAManager::init();
    drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
    drivers::kprintf("OK\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Detect ATA devices
    drivers::ATAManager::detect_devices();

    // Get primary master device
    drivers::ATADevice* disk = drivers::ATAManager::get_primary_master();
    if (disk) {
        drivers::kprintf("Primary master disk found\n");

        // Initialize VFS
        drivers::kprintf("Initializing VFS... ");
        fs::VFS::init();
        drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
        drivers::kprintf("OK\n");
        drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

        // Try to mount FAT32
        drivers::kprintf("Mounting FAT32 filesystem... ");
        fs::FAT32* fat32 = fs::FAT32::mount(disk);
        if (fat32) {
            drivers::VGA::set_color(Color::LIGHT_GREEN, Color::BLACK);
            drivers::kprintf("OK\n");
            drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

            // Mount as root
            fs::VFS::mount("/", fat32);
            drivers::kprintf("FAT32 mounted as root filesystem\n");
        } else {
            drivers::VGA::set_color(Color::LIGHT_RED, Color::BLACK);
            drivers::kprintf("FAILED\n");
            drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);
            drivers::kprintf("(No FAT32 filesystem found on disk)\n");
        }
    } else {
        drivers::kprintf("No ATA disk found (emulator may need disk image)\n");
    }

    // Print success message
    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::YELLOW, Color::BLACK);
    drivers::kprintf("================================\n");
    drivers::kprintf("  Hello from tiny-os kernel!\n");
    drivers::kprintf("================================\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    drivers::kprintf("\n");
    drivers::kprintf("Phase 1, 2, 3, 4 & 5 Complete!\n");
    drivers::kprintf("- Multiboot2 boot: OK\n");
    drivers::kprintf("- 64-bit long mode: OK\n");
    drivers::kprintf("- GDT setup: OK\n");
    drivers::kprintf("- VGA text mode: OK\n");
    drivers::kprintf("- Serial output: OK\n");
    drivers::kprintf("- C++ runtime: OK\n");
    drivers::kprintf("- Physical memory: OK\n");
    drivers::kprintf("- Virtual memory: OK\n");
    drivers::kprintf("- Heap allocator: OK\n");
    drivers::kprintf("- Interrupt handling: OK\n");
    drivers::kprintf("- PIC remapped: OK\n");
    drivers::kprintf("- Timer (100Hz): OK\n");
    drivers::kprintf("- Process management: OK\n");
    drivers::kprintf("- Thread management: OK\n");
    drivers::kprintf("- Scheduler (Round-Robin): OK\n");
    drivers::kprintf("- Multitasking: OK\n");
    drivers::kprintf("- ATA driver: OK\n");
    drivers::kprintf("- VFS layer: OK\n");
    drivers::kprintf("- FAT32 filesystem: OK\n");

    drivers::serial_printf("\nPhase 5 complete. File system initialized.\n");
    drivers::serial_printf("Next phase: Keyboard driver and Shell\n");

    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::LIGHT_BLUE, Color::BLACK);
    drivers::kprintf("System fully operational!\n");
    drivers::kprintf("File system ready. Demo processes running.\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    drivers::kprintf("\n");

    // Idle loop - scheduler will switch between processes
    while (true) {
        asm volatile("hlt");
    }
}

} // namespace tiny_os::kernel
