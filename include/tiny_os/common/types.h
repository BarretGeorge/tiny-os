#pragma once

#include <cstddef>
#include <cstdint>

namespace tiny_os {

// Basic type aliases
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using usize = size_t;
using isize = ptrdiff_t;

// Physical and virtual addresses
using PhysicalAddress = uint64;
using VirtualAddress = uint64;

// Page-related constants
constexpr usize PAGE_SIZE = 4096;
constexpr usize PAGE_SHIFT = 12;

// Alignment macros
constexpr VirtualAddress page_align_down(VirtualAddress addr) {
    return addr & ~(PAGE_SIZE - 1);
}

constexpr VirtualAddress page_align_up(VirtualAddress addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

constexpr usize pages_needed(usize size) {
    return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}

constexpr bool is_page_aligned(VirtualAddress addr) {
    return (addr & (PAGE_SIZE - 1)) == 0;
}

// Multiboot2 constants
constexpr uint32 MULTIBOOT2_MAGIC = 0x36d76289;

// Color codes for VGA text mode
enum class Color : uint8 {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    PINK = 13,
    YELLOW = 14,
    WHITE = 15,
};

// Utility functions
constexpr uint8 make_color(Color fg, Color bg) {
    return static_cast<uint8>(fg) | (static_cast<uint8>(bg) << 4);
}

// Port I/O (will be implemented in port_io.h)
namespace port {
    inline void outb(uint16 port, uint8 value) {
        asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    }

    inline uint8 inb(uint16 port) {
        uint8 value;
        asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
        return value;
    }

    inline void outw(uint16 port, uint16 value) {
        asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
    }

    inline uint16 inw(uint16 port) {
        uint16 value;
        asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
        return value;
    }

    inline void outl(uint16 port, uint32 value) {
        asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
    }

    inline uint32 inl(uint16 port) {
        uint32 value;
        asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
        return value;
    }
}

} // namespace tiny_os
