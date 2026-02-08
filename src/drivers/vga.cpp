#include <tiny_os/drivers/vga.h>
#include <tiny_os/common/string.h>
#include <cstdarg>

namespace tiny_os::drivers {

usize VGA::cursor_x_ = 0;
usize VGA::cursor_y_ = 0;
uint8 VGA::color_ = make_color(Color::LIGHT_GRAY, Color::BLACK);

void VGA::init() {
    cursor_x_ = 0;
    cursor_y_ = 0;
    color_ = make_color(Color::LIGHT_GRAY, Color::BLACK);
    clear();
}

void VGA::clear() {
    for (usize y = 0; y < HEIGHT; y++) {
        for (usize x = 0; x < WIDTH; x++) {
            const usize index = y * WIDTH + x;
            BUFFER[index] = make_entry(' ', color_);
        }
    }
    cursor_x_ = 0;
    cursor_y_ = 0;
    update_cursor();
}

void VGA::putchar(char c) {
    if (c == '\n') {
        cursor_x_ = 0;
        cursor_y_++;
    } else if (c == '\r') {
        cursor_x_ = 0;
    } else if (c == '\t') {
        cursor_x_ = (cursor_x_ + 4) & ~3;  // Align to 4
    } else if (c == '\b') {
        if (cursor_x_ > 0) {
            cursor_x_--;
        }
    } else {
        const usize index = cursor_y_ * WIDTH + cursor_x_;
        BUFFER[index] = make_entry(c, color_);
        cursor_x_++;
    }

    // Wrap to next line
    if (cursor_x_ >= WIDTH) {
        cursor_x_ = 0;
        cursor_y_++;
    }

    // Scroll if needed
    if (cursor_y_ >= HEIGHT) {
        scroll();
    }

    update_cursor();
}

void VGA::write(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

void VGA::write_hex(uint64 value) {
    char buffer[17];  // 16 hex digits + null
    utoa(value, buffer, 16);
    write("0x");
    write(buffer);
}

void VGA::write_dec(uint64 value) {
    char buffer[21];  // Max uint64 is 20 digits + null
    utoa(value, buffer, 10);
    write(buffer);
}

void VGA::set_color(Color fg, Color bg) {
    color_ = make_color(fg, bg);
}

void VGA::scroll() {
    // Move all lines up by one
    for (usize y = 0; y < HEIGHT - 1; y++) {
        for (usize x = 0; x < WIDTH; x++) {
            const usize src_index = (y + 1) * WIDTH + x;
            const usize dest_index = y * WIDTH + x;
            BUFFER[dest_index] = BUFFER[src_index];
        }
    }

    // Clear the last line
    for (usize x = 0; x < WIDTH; x++) {
        const usize index = (HEIGHT - 1) * WIDTH + x;
        BUFFER[index] = make_entry(' ', color_);
    }

    cursor_y_ = HEIGHT - 1;
}

void VGA::update_cursor() {
    uint16 pos = cursor_y_ * WIDTH + cursor_x_;

    // Write to VGA cursor registers
    port::outb(0x3D4, 0x0F);
    port::outb(0x3D5, static_cast<uint8>(pos & 0xFF));
    port::outb(0x3D4, 0x0E);
    port::outb(0x3D5, static_cast<uint8>((pos >> 8) & 0xFF));
}

uint16 VGA::make_entry(char c, uint8 color) {
    return static_cast<uint16>(c) | (static_cast<uint16>(color) << 8);
}

// Simple printf implementation
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int64 value = va_arg(args, int64);
                    char buffer[21];
                    itoa(value, buffer, 10);
                    VGA::write(buffer);
                    break;
                }
                case 'u': {
                    uint64 value = va_arg(args, uint64);
                    VGA::write_dec(value);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64 value = va_arg(args, uint64);
                    VGA::write_hex(value);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    VGA::write(str);
                    break;
                }
                case 'c': {
                    char c = static_cast<char>(va_arg(args, int));
                    VGA::putchar(c);
                    break;
                }
                case '%': {
                    VGA::putchar('%');
                    break;
                }
                default:
                    VGA::putchar('%');
                    VGA::putchar(*format);
                    break;
            }
        } else {
            VGA::putchar(*format);
        }
        format++;
    }

    va_end(args);
}

} // namespace tiny_os::drivers
