#include <tiny_os/drivers/timer.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/arch/x86_64/idt.h>
#include <tiny_os/arch/x86_64/pic.h>
#include <tiny_os/process/scheduler.h>

namespace tiny_os::drivers {

// Static member definitions
uint64 Timer::ticks_ = 0;
uint32 Timer::frequency_ = 0;

void Timer::init(uint32 frequency) {
    serial_printf("[Timer] Initializing PIT at %d Hz...\n", frequency);

    frequency_ = frequency;

    // Calculate divisor
    uint32 divisor = PIT_BASE_FREQ / frequency;

    // Send command byte: Channel 0, lobyte/hibyte, rate generator
    outb(PIT_COMMAND, 0x36);

    // Send divisor
    outb(PIT_CHANNEL0, divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  // High byte

    // Register IRQ0 handler
    arch::x86_64::IDT::register_handler(32, [](arch::x86_64::InterruptFrame* frame) {
        timer_interrupt_handler(frame);
    });

    // Unmask IRQ0
    arch::x86_64::PIC::unmask_irq(0);

    serial_printf("[Timer] PIT initialized at %d Hz (divisor: %d)\n", frequency, divisor);
    kprintf("[Timer] System timer initialized at %d Hz\n", frequency);
}

void Timer::timer_interrupt_handler(void* frame) {
    (void)frame;  // Unused

    ticks_++;

    // Print uptime every second (for debugging)
    if (ticks_ % frequency_ == 0) {
        uint64 seconds = ticks_ / frequency_;
        serial_printf("[Timer] Uptime: %d seconds (%d ticks)\n", seconds, ticks_);
    }

    // Send EOI to PIC
    arch::x86_64::PIC::send_eoi(0);

    // Call scheduler (Phase 4)
    process::Scheduler::schedule();
}

uint64 Timer::get_ticks() {
    return ticks_;
}

uint64 Timer::get_uptime_seconds() {
    if (frequency_ == 0) return 0;
    return ticks_ / frequency_;
}

void Timer::sleep_ms(uint32 milliseconds) {
    if (frequency_ == 0) return;

    uint64 target_ticks = ticks_ + (milliseconds * frequency_) / 1000;

    while (ticks_ < target_ticks) {
        asm volatile("hlt");  // Wait for interrupt
    }
}

} // namespace tiny_os::drivers
