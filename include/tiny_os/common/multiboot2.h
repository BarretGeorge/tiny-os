#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os {

// Multiboot2 tag types
enum class MultibootTagType : uint32 {
    END = 0,
    CMDLINE = 1,
    BOOT_LOADER_NAME = 2,
    MODULE = 3,
    BASIC_MEMINFO = 4,
    BOOTDEV = 5,
    MMAP = 6,
    VBE = 7,
    FRAMEBUFFER = 8,
    ELF_SECTIONS = 9,
    APM = 10,
    EFI32 = 11,
    EFI64 = 12,
    SMBIOS = 13,
    ACPI_OLD = 14,
    ACPI_NEW = 15,
    NETWORK = 16,
    EFI_MMAP = 17,
    EFI_BS = 18,
    EFI32_IH = 19,
    EFI64_IH = 20,
    LOAD_BASE_ADDR = 21,
};

// Memory map entry types
enum class MemoryType : uint32 {
    AVAILABLE = 1,
    RESERVED = 2,
    ACPI_RECLAIMABLE = 3,
    NVS = 4,
    BAD_RAM = 5,
};

// Multiboot2 tag header
struct MultibootTag {
    uint32 type;
    uint32 size;
} __attribute__((packed));

// Memory map tag
struct MultibootTagMmap {
    uint32 type;
    uint32 size;
    uint32 entry_size;
    uint32 entry_version;
} __attribute__((packed));

// Memory map entry
struct MultibootMmapEntry {
    uint64 addr;
    uint64 len;
    uint32 type;
    uint32 reserved;
} __attribute__((packed));

// Basic memory info tag
struct MultibootTagBasicMeminfo {
    uint32 type;
    uint32 size;
    uint32 mem_lower;  // KB of lower memory
    uint32 mem_upper;  // KB of upper memory
} __attribute__((packed));

class Multiboot2 {
public:
    static void parse(void* multiboot_info);

    static const MultibootTag* find_tag(MultibootTagType type);
    static void print_memory_map();

    static uint64 get_total_memory();
    static uint64 get_available_memory();

private:
    static void* info_ptr_;
    static uint32 total_size_;
};

} // namespace tiny_os
