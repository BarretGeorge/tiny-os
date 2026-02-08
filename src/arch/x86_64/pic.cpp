#include <tiny_os/arch/x86_64/pic.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>

namespace tiny_os::arch::x86_64 {

// Small delay for PIC operations (some hardware needs this)
static void io_wait() {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

void PIC::init() {
    drivers::serial_printf("[PIC] Initializing 8259 PIC...\n");

    // Save current masks
    uint8 mask1 = inb(PIC1_DATA);
    uint8 mask2 = inb(PIC2_DATA);

    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Set vector offsets (ICW2)
    outb(PIC1_DATA, PIC1_OFFSET);  // Master PIC: IRQ 0-7 → INT 32-39
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);  // Slave PIC: IRQ 8-15 → INT 40-47
    io_wait();

    // Tell Master PIC there's a slave at IRQ2 (ICW3)
    outb(PIC1_DATA, 0x04);  // Slave on IRQ2 (binary: 0000 0100)
    io_wait();
    outb(PIC2_DATA, 0x02);  // Slave cascade identity (binary: 0000 0010)
    io_wait();

    // Set 8086 mode (ICW4)
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);

    drivers::serial_printf("[PIC] PIC remapped: IRQ 0-7 → INT 32-39, IRQ 8-15 → INT 40-47\n");
    drivers::kprintf("[PIC] 8259 PIC initialized and remapped\n");
}

void PIC::send_eoi(uint8 irq) {
    // If IRQ came from slave PIC, send EOI to both
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to master
    outb(PIC1_COMMAND, PIC_EOI);
}

void PIC::mask_irq(uint8 irq) {
    uint16 port;
    uint8 value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

void PIC::unmask_irq(uint8 irq) {
    uint16 port;
    uint8 value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void PIC::disable_all() {
    outb(PIC1_DATA, 0xFF);  // Mask all IRQs on master
    outb(PIC2_DATA, 0xFF);  // Mask all IRQs on slave
}

uint16 PIC::get_mask() {
    uint8 mask1 = inb(PIC1_DATA);
    uint8 mask2 = inb(PIC2_DATA);
    return (mask2 << 8) | mask1;
}

uint16 PIC::read_irr() {
    outb(PIC1_COMMAND, 0x0A);
    outb(PIC2_COMMAND, 0x0A);
    uint8 irr1 = inb(PIC1_COMMAND);
    uint8 irr2 = inb(PIC2_COMMAND);
    return (irr2 << 8) | irr1;
}

uint16 PIC::read_isr() {
    outb(PIC1_COMMAND, 0x0B);
    outb(PIC2_COMMAND, 0x0B);
    uint8 isr1 = inb(PIC1_COMMAND);
    uint8 isr2 = inb(PIC2_COMMAND);
    return (isr2 << 8) | isr1;
}

} // namespace tiny_os::arch::x86_64
