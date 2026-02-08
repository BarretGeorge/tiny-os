#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::drivers {

// PIT (Programmable Interval Timer) driver
class Timer {
public:
    // Initialize timer with specified frequency (in Hz)
    static void init(uint32 frequency);

    // Get number of ticks since boot
    static uint64 get_ticks();

    // Get uptime in seconds
    static uint64 get_uptime_seconds();

    // Sleep for specified milliseconds (busy wait for now)
    static void sleep_ms(uint32 milliseconds);

private:
    // PIT I/O ports
    static constexpr uint16 PIT_CHANNEL0 = 0x40;
    static constexpr uint16 PIT_COMMAND = 0x43;

    // PIT base frequency (1.193182 MHz)
    static constexpr uint32 PIT_BASE_FREQ = 1193182;

    // Timer interrupt handler
    static void timer_interrupt_handler(void* frame);

    static uint64 ticks_;
    static uint32 frequency_;
};

} // namespace tiny_os::drivers
