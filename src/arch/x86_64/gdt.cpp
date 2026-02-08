#include <tiny_os/arch/x86_64/gdt.h>
#include <tiny_os/common/string.h>

namespace tiny_os::arch::x86_64 {

GDTEntry GDT::entries_[GDT_ENTRIES];
GDTPointer GDT::pointer_;

void GDT::init() {
    pointer_.limit = sizeof(entries_) - 1;
    pointer_.base = reinterpret_cast<uint64>(&entries_);

    // Null descriptor
    set_gate(0, 0, 0, 0, 0);

    // Kernel code segment (64-bit)
    // Access: Present, Ring 0, Code, Executable, Readable
    // Granularity: 64-bit, Page granularity
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xA0);

    // Kernel data segment
    // Access: Present, Ring 0, Data, Writable
    set_gate(2, 0, 0xFFFFF, 0x92, 0xC0);

    // User code segment (64-bit)
    // Access: Present, Ring 3, Code, Executable, Readable
    set_gate(3, 0, 0xFFFFF, 0xFA, 0xA0);

    // User data segment
    // Access: Present, Ring 3, Data, Writable
    set_gate(4, 0, 0xFFFFF, 0xF2, 0xC0);

    // Load GDT
    gdt_load(&pointer_);
}

void GDT::set_gate(uint32 num, uint32 base, uint32 limit, uint8 access, uint8 gran) {
    entries_[num].base_low = base & 0xFFFF;
    entries_[num].base_middle = (base >> 16) & 0xFF;
    entries_[num].base_high = (base >> 24) & 0xFF;

    entries_[num].limit_low = limit & 0xFFFF;
    entries_[num].granularity = (limit >> 16) & 0x0F;
    entries_[num].granularity |= gran & 0xF0;

    entries_[num].access = access;
}

} // namespace tiny_os::arch::x86_64
