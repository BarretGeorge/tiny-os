#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::memory {

// Page table entry flags
namespace PageFlags {
    constexpr uint64 PRESENT = 1ULL << 0;
    constexpr uint64 WRITABLE = 1ULL << 1;
    constexpr uint64 USER = 1ULL << 2;
    constexpr uint64 WRITE_THROUGH = 1ULL << 3;
    constexpr uint64 CACHE_DISABLE = 1ULL << 4;
    constexpr uint64 ACCESSED = 1ULL << 5;
    constexpr uint64 DIRTY = 1ULL << 6;
    constexpr uint64 HUGE_PAGE = 1ULL << 7;
    constexpr uint64 GLOBAL = 1ULL << 8;
    constexpr uint64 NO_EXECUTE = 1ULL << 63;
}

// Page table entry (64-bit)
struct PageTableEntry {
    uint64 value;

    bool is_present() const { return value & PageFlags::PRESENT; }
    bool is_writable() const { return value & PageFlags::WRITABLE; }
    bool is_user() const { return value & PageFlags::USER; }
    bool is_huge() const { return value & PageFlags::HUGE_PAGE; }

    PhysicalAddress get_address() const {
        return value & 0x000FFFFFFFFFF000ULL;
    }

    void set_address(PhysicalAddress addr, uint64 flags) {
        value = (addr & 0x000FFFFFFFFFF000ULL) | flags;
    }

    void clear() {
        value = 0;
    }
} __attribute__((packed));

static_assert(sizeof(PageTableEntry) == 8, "PageTableEntry must be 8 bytes");

// Page table (512 entries = 4KB)
struct PageTable {
    PageTableEntry entries[512];

    PageTableEntry& operator[](usize index) {
        return entries[index];
    }

    const PageTableEntry& operator[](usize index) const {
        return entries[index];
    }

    void clear() {
        for (usize i = 0; i < 512; i++) {
            entries[i].clear();
        }
    }
} __attribute__((aligned(4096)));

static_assert(sizeof(PageTable) == 4096, "PageTable must be 4KB");

// Extract indices from virtual address
struct PageTableIndices {
    uint16 pml4;
    uint16 pdpt;
    uint16 pd;
    uint16 pt;
    uint16 offset;

    static PageTableIndices from_address(VirtualAddress addr) {
        PageTableIndices indices;
        indices.offset = addr & 0xFFF;
        indices.pt = (addr >> 12) & 0x1FF;
        indices.pd = (addr >> 21) & 0x1FF;
        indices.pdpt = (addr >> 30) & 0x1FF;
        indices.pml4 = (addr >> 39) & 0x1FF;
        return indices;
    }
};

} // namespace tiny_os::memory
