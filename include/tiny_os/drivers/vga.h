#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::drivers {

class VGA {
public:
    static void init();
    static void clear();
    static void putchar(char c);
    static void write(const char* str);
    static void write_hex(uint64 value);
    static void write_dec(uint64 value);
    static void set_color(Color fg, Color bg);

private:
    static constexpr uint16* BUFFER = reinterpret_cast<uint16*>(0xB8000);
    static constexpr usize WIDTH = 80;
    static constexpr usize HEIGHT = 25;

    static usize cursor_x_;
    static usize cursor_y_;
    static uint8 color_;

    static void scroll();
    static void update_cursor();
    static uint16 make_entry(char c, uint8 color);
};

// Printf-like function (simple implementation)
void kprintf(const char* format, ...);

} // namespace tiny_os::drivers
