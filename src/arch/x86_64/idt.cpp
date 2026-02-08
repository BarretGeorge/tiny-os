#include <tiny_os/arch/x86_64/idt.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::arch::x86_64 {

// Static member definitions
IDTEntry IDT::entries_[IDT_ENTRIES];
InterruptHandler IDT::handlers_[IDT_ENTRIES];
IDTPointer IDT::idtr_;

void IDT::init() {
    drivers::serial_printf("[IDT] Initializing Interrupt Descriptor Table...\n");

    // Clear all entries and handlers
    for (usize i = 0; i < IDT_ENTRIES; i++) {
        entries_[i] = {};
        handlers_[i] = nullptr;
    }

    // Set up IDT pointer
    idtr_.limit = sizeof(entries_) - 1;
    idtr_.base = reinterpret_cast<uint64>(&entries_[0]);

    // Install ISR handlers for CPU exceptions (0-31)
    set_gate(0, isr0, IDTType::INTERRUPT_GATE);
    set_gate(1, isr1, IDTType::INTERRUPT_GATE);
    set_gate(2, isr2, IDTType::INTERRUPT_GATE);
    set_gate(3, isr3, IDTType::INTERRUPT_GATE);
    set_gate(4, isr4, IDTType::INTERRUPT_GATE);
    set_gate(5, isr5, IDTType::INTERRUPT_GATE);
    set_gate(6, isr6, IDTType::INTERRUPT_GATE);
    set_gate(7, isr7, IDTType::INTERRUPT_GATE);
    set_gate(8, isr8, IDTType::INTERRUPT_GATE);
    set_gate(9, isr9, IDTType::INTERRUPT_GATE);
    set_gate(10, isr10, IDTType::INTERRUPT_GATE);
    set_gate(11, isr11, IDTType::INTERRUPT_GATE);
    set_gate(12, isr12, IDTType::INTERRUPT_GATE);
    set_gate(13, isr13, IDTType::INTERRUPT_GATE);
    set_gate(14, isr14, IDTType::INTERRUPT_GATE);
    set_gate(15, isr15, IDTType::INTERRUPT_GATE);
    set_gate(16, isr16, IDTType::INTERRUPT_GATE);
    set_gate(17, isr17, IDTType::INTERRUPT_GATE);
    set_gate(18, isr18, IDTType::INTERRUPT_GATE);
    set_gate(19, isr19, IDTType::INTERRUPT_GATE);
    set_gate(20, isr20, IDTType::INTERRUPT_GATE);
    set_gate(21, isr21, IDTType::INTERRUPT_GATE);
    set_gate(22, isr22, IDTType::INTERRUPT_GATE);
    set_gate(23, isr23, IDTType::INTERRUPT_GATE);
    set_gate(24, isr24, IDTType::INTERRUPT_GATE);
    set_gate(25, isr25, IDTType::INTERRUPT_GATE);
    set_gate(26, isr26, IDTType::INTERRUPT_GATE);
    set_gate(27, isr27, IDTType::INTERRUPT_GATE);
    set_gate(28, isr28, IDTType::INTERRUPT_GATE);
    set_gate(29, isr29, IDTType::INTERRUPT_GATE);
    set_gate(30, isr30, IDTType::INTERRUPT_GATE);
    set_gate(31, isr31, IDTType::INTERRUPT_GATE);

    // Install IRQ handlers (32-47)
    set_gate(32, irq0, IDTType::INTERRUPT_GATE);
    set_gate(33, irq1, IDTType::INTERRUPT_GATE);
    set_gate(34, irq2, IDTType::INTERRUPT_GATE);
    set_gate(35, irq3, IDTType::INTERRUPT_GATE);
    set_gate(36, irq4, IDTType::INTERRUPT_GATE);
    set_gate(37, irq5, IDTType::INTERRUPT_GATE);
    set_gate(38, irq6, IDTType::INTERRUPT_GATE);
    set_gate(39, irq7, IDTType::INTERRUPT_GATE);
    set_gate(40, irq8, IDTType::INTERRUPT_GATE);
    set_gate(41, irq9, IDTType::INTERRUPT_GATE);
    set_gate(42, irq10, IDTType::INTERRUPT_GATE);
    set_gate(43, irq11, IDTType::INTERRUPT_GATE);
    set_gate(44, irq12, IDTType::INTERRUPT_GATE);
    set_gate(45, irq13, IDTType::INTERRUPT_GATE);
    set_gate(46, irq14, IDTType::INTERRUPT_GATE);
    set_gate(47, irq15, IDTType::INTERRUPT_GATE);

    // Install system call handler (0x80 = 128)
    set_gate(128, isr128, IDTType::USER_INTERRUPT);

    // Load the IDT
    asm volatile("lidt %0" : : "m"(idtr_));

    drivers::serial_printf("[IDT] IDT loaded with %d entries\n", IDT_ENTRIES);
    drivers::kprintf("[IDT] Interrupt Descriptor Table initialized\n");
}

void IDT::set_gate(uint8 num, void (*handler)(), uint8 type) {
    uint64 handler_addr = reinterpret_cast<uint64>(handler);

    entries_[num].offset_low = handler_addr & 0xFFFF;
    entries_[num].offset_mid = (handler_addr >> 16) & 0xFFFF;
    entries_[num].offset_high = (handler_addr >> 32) & 0xFFFFFFFF;

    entries_[num].selector = 0x08;  // Kernel code segment
    entries_[num].ist = 0;          // No IST
    entries_[num].type_attr = type;
    entries_[num].reserved = 0;
}

void IDT::register_handler(uint8 num, InterruptHandler handler) {
    handlers_[num] = handler;
}

void IDT::enable_interrupts() {
    asm volatile("sti");
}

void IDT::disable_interrupts() {
    asm volatile("cli");
}

bool IDT::are_interrupts_enabled() {
    uint64 rflags;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    return (rflags & 0x200) != 0;  // Check IF flag
}

// Exception names
const char* get_exception_name(uint8 exception) {
    static const char* exception_names[] = {
        "Divide by Zero",
        "Debug",
        "Non-Maskable Interrupt",
        "Breakpoint",
        "Overflow",
        "Bound Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack-Segment Fault",
        "General Protection Fault",
        "Page Fault",
        "Reserved",
        "x87 Floating-Point Exception",
        "Alignment Check",
        "Machine Check",
        "SIMD Floating-Point Exception",
        "Virtualization Exception",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Security Exception",
        "Reserved"
    };

    if (exception < 32) {
        return exception_names[exception];
    }
    return "Unknown Exception";
}

} // namespace tiny_os::arch::x86_64

// Default exception handler
static void default_exception_handler(tiny_os::arch::x86_64::InterruptFrame* frame) {
    using namespace tiny_os;

    drivers::kprintf("\n=== CPU EXCEPTION ===\n");
    drivers::kprintf("Exception %d: %s\n", frame->int_no,
                    arch::x86_64::get_exception_name(frame->int_no));
    drivers::kprintf("Error Code: 0x%lx\n", frame->err_code);
    drivers::kprintf("\nRegisters:\n");
    drivers::kprintf("  RIP: 0x%016lx  RSP: 0x%016lx\n", frame->rip, frame->rsp);
    drivers::kprintf("  RAX: 0x%016lx  RBX: 0x%016lx\n", frame->rax, frame->rbx);
    drivers::kprintf("  RCX: 0x%016lx  RDX: 0x%016lx\n", frame->rcx, frame->rdx);
    drivers::kprintf("  RSI: 0x%016lx  RDI: 0x%016lx\n", frame->rsi, frame->rdi);
    drivers::kprintf("  RBP: 0x%016lx  CS:  0x%04lx\n", frame->rbp, frame->cs);
    drivers::kprintf("  RFLAGS: 0x%016lx\n", frame->rflags);

    // Page fault specific info
    if (frame->int_no == 14) {
        uint64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        drivers::kprintf("\nPage Fault Address: 0x%016lx\n", cr2);
        drivers::kprintf("Caused by: ");
        if (!(frame->err_code & 0x1)) drivers::kprintf("Page not present ");
        if (frame->err_code & 0x2) drivers::kprintf("Write ");
        else drivers::kprintf("Read ");
        if (frame->err_code & 0x4) drivers::kprintf("(User mode)");
        else drivers::kprintf("(Kernel mode)");
        drivers::kprintf("\n");
    }

    drivers::kprintf("\n=== KERNEL PANIC ===\n");
    drivers::kprintf("System halted.\n");

    // Halt the system
    while (true) {
        asm volatile("cli; hlt");
    }
}

// Common interrupt dispatcher (called from assembly)
extern "C" void interrupt_dispatcher(tiny_os::arch::x86_64::InterruptFrame* frame) {
    using namespace tiny_os::arch::x86_64;

    // Check if a custom handler is registered
    if (IDT::handlers_[frame->int_no]) {
        IDT::handlers_[frame->int_no](frame);
    } else if (frame->int_no < 32) {
        // Unhandled CPU exception - use default handler
        default_exception_handler(frame);
    }
    // For IRQs without handlers, just ignore (will be handled by PIC)
}
