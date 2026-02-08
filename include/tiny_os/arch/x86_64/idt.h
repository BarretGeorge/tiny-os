#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::arch::x86_64 {

// IDT entry (16 bytes)
struct IDTEntry {
    uint16 offset_low;      // Offset bits 0-15
    uint16 selector;        // Code segment selector
    uint8 ist;              // Interrupt Stack Table offset
    uint8 type_attr;        // Type and attributes
    uint16 offset_mid;      // Offset bits 16-31
    uint32 offset_high;     // Offset bits 32-63
    uint32 reserved;        // Reserved (must be 0)
} __attribute__((packed));

static_assert(sizeof(IDTEntry) == 16, "IDTEntry must be 16 bytes");

// IDT pointer structure
struct IDTPointer {
    uint16 limit;           // Size of IDT - 1
    uint64 base;            // Base address of IDT
} __attribute__((packed));

// IDT gate types
namespace IDTType {
    constexpr uint8 INTERRUPT_GATE = 0x8E;  // 64-bit interrupt gate (P=1, DPL=0)
    constexpr uint8 TRAP_GATE = 0x8F;       // 64-bit trap gate (P=1, DPL=0)
    constexpr uint8 USER_INTERRUPT = 0xEE;  // User-mode interrupt (P=1, DPL=3)
}

// Interrupt frame (pushed by CPU and our ISR stub)
struct InterruptFrame {
    // Pushed by our ISR stub
    uint64 r15, r14, r13, r12, r11, r10, r9, r8;
    uint64 rbp, rdi, rsi, rdx, rcx, rbx, rax;

    // Pushed by ISR common stub
    uint64 int_no;          // Interrupt number
    uint64 err_code;        // Error code (or 0)

    // Pushed by CPU
    uint64 rip;             // Instruction pointer
    uint64 cs;              // Code segment
    uint64 rflags;          // CPU flags
    uint64 rsp;             // Stack pointer
    uint64 ss;              // Stack segment
} __attribute__((packed));

// Interrupt handler function type
using InterruptHandler = void (*)(InterruptFrame* frame);

class IDT {
public:
    // Initialize the IDT
    static void init();

    // Set an IDT gate
    static void set_gate(uint8 num, void (*handler)(), uint8 type);

    // Register a C++ interrupt handler
    static void register_handler(uint8 num, InterruptHandler handler);

    // Enable/disable interrupts
    static void enable_interrupts();
    static void disable_interrupts();
    static bool are_interrupts_enabled();

private:
    static constexpr usize IDT_ENTRIES = 256;

    static IDTEntry entries_[IDT_ENTRIES];
    static InterruptHandler handlers_[IDT_ENTRIES];
    static IDTPointer idtr_;
};

// Exception names
const char* get_exception_name(uint8 exception);

} // namespace tiny_os::arch::x86_64

// Assembly ISR stubs (defined in idt.asm)
extern "C" {
    // ISR handlers for CPU exceptions (0-31)
    void isr0();   void isr1();   void isr2();   void isr3();
    void isr4();   void isr5();   void isr6();   void isr7();
    void isr8();   void isr9();   void isr10();  void isr11();
    void isr12();  void isr13();  void isr14();  void isr15();
    void isr16();  void isr17();  void isr18();  void isr19();
    void isr20();  void isr21();  void isr22();  void isr23();
    void isr24();  void isr25();  void isr26();  void isr27();
    void isr28();  void isr29();  void isr30();  void isr31();

    // IRQ handlers (32-47)
    void irq0();   void irq1();   void irq2();   void irq3();
    void irq4();   void irq5();   void irq6();   void irq7();
    void irq8();   void irq9();   void irq10();  void irq11();
    void irq12();  void irq13();  void irq14();  void irq15();

    // Generic ISR handlers (48-255)
    void isr128(); // System call interrupt

    // Common interrupt dispatcher (called from assembly)
    void interrupt_dispatcher(InterruptFrame* frame);
}
