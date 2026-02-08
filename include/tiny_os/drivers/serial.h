#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::drivers {

class Serial {
public:
    static void init();
    static void write(char c);
    static void write(const char* str);
    static bool can_write();

private:
    static constexpr uint16 COM1 = 0x3F8;

    static bool is_transmit_empty();
};

// Serial printf for debugging
void serial_printf(const char* format, ...);

} // namespace tiny_os::drivers
