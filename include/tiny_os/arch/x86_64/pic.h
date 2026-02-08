#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::arch::x86_64 {

// 8259 PIC (Programmable Interrupt Controller) driver
class PIC {
public:
    // Initialize and remap the PIC
    // Remaps IRQ 0-7 to interrupts 32-39
    // Remaps IRQ 8-15 to interrupts 40-47
    static void init();

    // Send End of Interrupt (EOI) signal
    static void send_eoi(uint8 irq);

    // Mask/unmask specific IRQ
    static void mask_irq(uint8 irq);
    static void unmask_irq(uint8 irq);

    // Disable all IRQs
    static void disable_all();

    // Get IRQ mask
    static uint16 get_mask();

private:
    // PIC I/O ports
    static constexpr uint16 PIC1_COMMAND = 0x20;
    static constexpr uint16 PIC1_DATA = 0x21;
    static constexpr uint16 PIC2_COMMAND = 0xA0;
    static constexpr uint16 PIC2_DATA = 0xA1;

    // PIC commands
    static constexpr uint8 PIC_EOI = 0x20;
    static constexpr uint8 ICW1_INIT = 0x10;
    static constexpr uint8 ICW1_ICW4 = 0x01;
    static constexpr uint8 ICW4_8086 = 0x01;

    // IRQ offsets (where IRQs are remapped to)
    static constexpr uint8 PIC1_OFFSET = 32;
    static constexpr uint8 PIC2_OFFSET = 40;

    // Read IRR (Interrupt Request Register)
    static uint16 read_irr();

    // Read ISR (In-Service Register)
    static uint16 read_isr();
};

} // namespace tiny_os::arch::x86_64
