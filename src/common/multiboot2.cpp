#include <tiny_os/common/multiboot2.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>

namespace tiny_os {

void* Multiboot2::info_ptr_ = nullptr;
uint32 Multiboot2::total_size_ = 0;

void Multiboot2::parse(void* multiboot_info) {
    info_ptr_ = multiboot_info;
    total_size_ = *reinterpret_cast<uint32*>(multiboot_info);

    drivers::serial_printf("Multiboot2 info size: %u bytes\n", total_size_);
}

const MultibootTag* Multiboot2::find_tag(MultibootTagType type) {
    if (!info_ptr_) return nullptr;

    auto* tag = reinterpret_cast<MultibootTag*>(
        reinterpret_cast<uint8*>(info_ptr_) + 8);

    while (tag->type != static_cast<uint32>(MultibootTagType::END)) {
        if (tag->type == static_cast<uint32>(type)) {
            return tag;
        }

        // Align to 8-byte boundary
        tag = reinterpret_cast<MultibootTag*>(
            reinterpret_cast<uint8*>(tag) + ((tag->size + 7) & ~7));
    }

    return nullptr;
}

void Multiboot2::print_memory_map() {
    auto* mmap_tag = reinterpret_cast<const MultibootTagMmap*>(
        find_tag(MultibootTagType::MMAP));

    if (!mmap_tag) {
        drivers::kprintf("No memory map found!\n");
        return;
    }

    drivers::kprintf("\n=== Memory Map ===\n");
    drivers::serial_printf("\n=== Memory Map ===\n");

    auto* entry = reinterpret_cast<const MultibootMmapEntry*>(
        reinterpret_cast<const uint8*>(mmap_tag) + sizeof(MultibootTagMmap));

    const uint8* end = reinterpret_cast<const uint8*>(mmap_tag) + mmap_tag->size;

    while (reinterpret_cast<const uint8*>(entry) < end) {
        const char* type_str = "UNKNOWN";
        switch (static_cast<MemoryType>(entry->type)) {
            case MemoryType::AVAILABLE:
                type_str = "AVAILABLE";
                break;
            case MemoryType::RESERVED:
                type_str = "RESERVED";
                break;
            case MemoryType::ACPI_RECLAIMABLE:
                type_str = "ACPI_RECLAIM";
                break;
            case MemoryType::NVS:
                type_str = "NVS";
                break;
            case MemoryType::BAD_RAM:
                type_str = "BAD_RAM";
                break;
        }

        drivers::kprintf("  0x%x - 0x%x (%u KB) - %s\n",
                        entry->addr,
                        entry->addr + entry->len - 1,
                        entry->len / 1024,
                        type_str);

        drivers::serial_printf("  0x%016lx - 0x%016lx (%lu KB) - %s\n",
                              entry->addr,
                              entry->addr + entry->len - 1,
                              entry->len / 1024,
                              type_str);

        entry = reinterpret_cast<const MultibootMmapEntry*>(
            reinterpret_cast<const uint8*>(entry) + mmap_tag->entry_size);
    }
}

uint64 Multiboot2::get_total_memory() {
    auto* mmap_tag = reinterpret_cast<const MultibootTagMmap*>(
        find_tag(MultibootTagType::MMAP));

    if (!mmap_tag) return 0;

    uint64 total = 0;
    auto* entry = reinterpret_cast<const MultibootMmapEntry*>(
        reinterpret_cast<const uint8*>(mmap_tag) + sizeof(MultibootTagMmap));

    const uint8* end = reinterpret_cast<const uint8*>(mmap_tag) + mmap_tag->size;

    while (reinterpret_cast<const uint8*>(entry) < end) {
        total += entry->len;
        entry = reinterpret_cast<const MultibootMmapEntry*>(
            reinterpret_cast<const uint8*>(entry) + mmap_tag->entry_size);
    }

    return total;
}

uint64 Multiboot2::get_available_memory() {
    auto* mmap_tag = reinterpret_cast<const MultibootTagMmap*>(
        find_tag(MultibootTagType::MMAP));

    if (!mmap_tag) return 0;

    uint64 available = 0;
    auto* entry = reinterpret_cast<const MultibootMmapEntry*>(
        reinterpret_cast<const uint8*>(mmap_tag) + sizeof(MultibootTagMmap));

    const uint8* end = reinterpret_cast<const uint8*>(mmap_tag) + mmap_tag->size;

    while (reinterpret_cast<const uint8*>(entry) < end) {
        if (entry->type == static_cast<uint32>(MemoryType::AVAILABLE)) {
            available += entry->len;
        }
        entry = reinterpret_cast<const MultibootMmapEntry*>(
            reinterpret_cast<const uint8*>(entry) + mmap_tag->entry_size);
    }

    return available;
}

} // namespace tiny_os
