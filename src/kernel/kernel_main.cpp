#include <tiny_os/kernel/kernel.h>
#include <tiny_os/arch/x86_64/gdt.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>

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
    drivers::kprintf("   tiny-os v0.1.0 - Phase 1\n");
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

    // Print success message
    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::YELLOW, Color::BLACK);
    drivers::kprintf("================================\n");
    drivers::kprintf("  Hello from tiny-os kernel!\n");
    drivers::kprintf("================================\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    drivers::kprintf("\n");
    drivers::kprintf("Phase 1 Complete!\n");
    drivers::kprintf("- Multiboot2 boot: OK\n");
    drivers::kprintf("- 64-bit long mode: OK\n");
    drivers::kprintf("- GDT setup: OK\n");
    drivers::kprintf("- VGA text mode: OK\n");
    drivers::kprintf("- Serial output: OK\n");
    drivers::kprintf("- C++ runtime: OK\n");

    drivers::serial_printf("\nPhase 1 complete. Kernel is running in 64-bit mode.\n");
    drivers::serial_printf("Next phase: Memory management\n");

    drivers::kprintf("\n");
    drivers::VGA::set_color(Color::LIGHT_BLUE, Color::BLACK);
    drivers::kprintf("System halted. Press Reset to reboot.\n");
    drivers::VGA::set_color(Color::LIGHT_GRAY, Color::BLACK);

    // Halt the system
    while (true) {
        asm volatile("hlt");
    }
}

} // namespace tiny_os::kernel
