#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>
#include <cstdarg>

namespace tiny_os::drivers {

void Serial::init() {
    // Disable interrupts
    port::outb(COM1 + 1, 0x00);

    // Enable DLAB (set baud rate divisor)
    port::outb(COM1 + 3, 0x80);

    // Set divisor to 3 (38400 baud)
    port::outb(COM1 + 0, 0x03);
    port::outb(COM1 + 1, 0x00);

    // 8 bits, no parity, one stop bit
    port::outb(COM1 + 3, 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    port::outb(COM1 + 2, 0xC7);

    // IRQs enabled, RTS/DSR set
    port::outb(COM1 + 4, 0x0B);

    // Enable interrupts
    port::outb(COM1 + 1, 0x01);
}

bool Serial::is_transmit_empty() {
    return (port::inb(COM1 + 5) & 0x20) != 0;
}

bool Serial::can_write() {
    return is_transmit_empty();
}

void Serial::write(char c) {
    // Wait for transmit to be empty
    while (!is_transmit_empty()) {
        // Busy wait
    }

    port::outb(COM1, c);
}

void Serial::write(const char* str) {
    while (*str) {
        write(*str++);
    }
}

void serial_printf(const char* format, ...) {
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
                    Serial::write(buffer);
                    break;
                }
                case 'u': {
                    uint64 value = va_arg(args, uint64);
                    char buffer[21];
                    utoa(value, buffer, 10);
                    Serial::write(buffer);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64 value = va_arg(args, uint64);
                    char buffer[17];
                    utoa(value, buffer, 16);
                    Serial::write("0x");
                    Serial::write(buffer);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    Serial::write(str);
                    break;
                }
                case 'c': {
                    char c = static_cast<char>(va_arg(args, int));
                    Serial::write(c);
                    break;
                }
                case '%': {
                    Serial::write('%');
                    break;
                }
                default:
                    Serial::write('%');
                    Serial::write(*format);
                    break;
            }
        } else {
            Serial::write(*format);
        }
        format++;
    }

    va_end(args);
}

} // namespace tiny_os::drivers
