#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::arch::x86_64 {

// GDT Entry structure (8 bytes)
struct GDTEntry {
    uint16 limit_low;
    uint16 base_low;
    uint8 base_middle;
    uint8 access;
    uint8 granularity;
    uint8 base_high;
} __attribute__((packed));

// GDT Pointer structure
struct GDTPointer {
    uint16 limit;
    uint64 base;
} __attribute__((packed));

class GDT {
public:
    static void init();

private:
    static constexpr usize GDT_ENTRIES = 5;
    static GDTEntry entries_[GDT_ENTRIES];
    static GDTPointer pointer_;

    static void set_gate(uint32 num, uint32 base, uint32 limit, uint8 access, uint8 gran);
};

// Assembly function to load GDT
extern "C" void gdt_load(GDTPointer* pointer);

} // namespace tiny_os::arch::x86_64
