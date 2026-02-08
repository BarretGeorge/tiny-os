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
    drivers::kprintf("   tiny-os v0.1.0 - Phase 3\n");
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

    // Print success message
    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::YELLOW, Color::BLACK);
    drivers::kprintf("================================\n");
    drivers::kprintf("  Hello from tiny-os kernel!\n");
    drivers::kprintf("================================\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    drivers::kprintf("\n");
    drivers::kprintf("Phase 1, 2 & 3 Complete!\n");
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

    drivers::serial_printf("\nPhase 3 complete. Interrupt handling initialized.\n");
    drivers::serial_printf("Next phase: Process and thread management\n");

    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::LIGHT_BLUE, Color::BLACK);
    drivers::kprintf("Timer is running. Uptime displayed every second.\n");
    drivers::kprintf("Press Reset to reboot.\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Idle loop - timer interrupts will fire
    while (true) {
        asm volatile("hlt");

        // Display uptime on screen every 5 seconds
        static uint64 last_displayed = 0;
        uint64 uptime = drivers::Timer::get_uptime_seconds();
        if (uptime > 0 && uptime != last_displayed && uptime % 5 == 0) {
            last_displayed = uptime;
            drivers::kprintf("Uptime: %d seconds (%d ticks)\n",
                           uptime, drivers::Timer::get_ticks());
        }
    }
}

} // namespace tiny_os::kernel
