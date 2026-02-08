#include <tiny_os/memory/physical_allocator.h>
#include <tiny_os/common/multiboot2.h>
#include <tiny_os/common/string.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/kernel/kernel.h>

namespace tiny_os::memory {

uint32* PhysicalAllocator::bitmap_ = nullptr;
usize PhysicalAllocator::bitmap_size_ = 0;
usize PhysicalAllocator::total_frames_ = 0;
usize PhysicalAllocator::used_frames_ = 0;
PhysicalAddress PhysicalAllocator::memory_end_ = 0;

// External symbol from linker script
extern "C" uint8 kernel_physical_end;

void PhysicalAllocator::init(void* multiboot_info) {
    drivers::kprintf("Initializing physical memory allocator...\n");
    drivers::serial_printf("Physical allocator init\n");

    // Parse multiboot info
    Multiboot2::parse(multiboot_info);

    // Print memory map
    Multiboot2::print_memory_map();

    // Get total available memory
    uint64 total_mem = Multiboot2::get_total_memory();
    uint64 available_mem = Multiboot2::get_available_memory();

    drivers::kprintf("\nTotal memory: %u MB\n", total_mem / (1024 * 1024));
    drivers::kprintf("Available memory: %u MB\n", available_mem / (1024 * 1024));
    drivers::serial_printf("Total: %lu bytes, Available: %lu bytes\n",
                          total_mem, available_mem);

    // Calculate number of frames (assume max 4GB for now)
    memory_end_ = 0x100000000ULL;  // 4GB
    total_frames_ = memory_end_ / FRAME_SIZE;

    // Calculate bitmap size in uint32s
    bitmap_size_ = (total_frames_ + BITMAP_ENTRIES_PER_UINT32 - 1) /
                   BITMAP_ENTRIES_PER_UINT32;

    // Place bitmap after kernel end
    PhysicalAddress kernel_end_phys =
        reinterpret_cast<PhysicalAddress>(&kernel_physical_end);
    bitmap_ = reinterpret_cast<uint32*>(kernel_end_phys);

    drivers::serial_printf("Kernel ends at: 0x%lx\n", kernel_end_phys);
    drivers::serial_printf("Bitmap at: 0x%lx, size: %lu bytes\n",
                          reinterpret_cast<uint64>(bitmap_),
                          bitmap_size_ * sizeof(uint32));

    // Initialize bitmap (mark all as used)
    memset(bitmap_, 0xFF, bitmap_size_ * sizeof(uint32));
    used_frames_ = total_frames_;

    // Mark available memory regions as free
    auto* mmap_tag = reinterpret_cast<const MultibootTagMmap*>(
        Multiboot2::find_tag(MultibootTagType::MMAP));

    if (!mmap_tag) {
        kernel::panic("No memory map found!");
    }

    auto* entry = reinterpret_cast<const MultibootMmapEntry*>(
        reinterpret_cast<const uint8*>(mmap_tag) + sizeof(MultibootTagMmap));

    const uint8* end = reinterpret_cast<const uint8*>(mmap_tag) + mmap_tag->size;

    while (reinterpret_cast<const uint8*>(entry) < end) {
        if (entry->type == static_cast<uint32>(MemoryType::AVAILABLE)) {
            PhysicalAddress start = entry->addr;
            PhysicalAddress end_addr = entry->addr + entry->len;

            // Align to frame boundaries
            start = page_align_up(start);
            end_addr = page_align_down(end_addr);

            // Free each frame in this region
            for (PhysicalAddress addr = start; addr < end_addr; addr += FRAME_SIZE) {
                usize frame = addr / FRAME_SIZE;
                if (frame < total_frames_) {
                    clear_frame(frame);
                    used_frames_--;
                }
            }
        }

        entry = reinterpret_cast<const MultibootMmapEntry*>(
            reinterpret_cast<const uint8*>(entry) + mmap_tag->entry_size);
    }

    // Mark first 1MB as used (BIOS, VGA, etc.)
    for (usize frame = 0; frame < 0x100000 / FRAME_SIZE; frame++) {
        if (!test_frame(frame)) {
            set_frame(frame);
            used_frames_++;
        }
    }

    // Mark kernel and bitmap area as used
    PhysicalAddress bitmap_end = reinterpret_cast<PhysicalAddress>(bitmap_) +
                                  bitmap_size_ * sizeof(uint32);
    for (PhysicalAddress addr = 0x100000; addr < bitmap_end; addr += FRAME_SIZE) {
        usize frame = addr / FRAME_SIZE;
        if (!test_frame(frame)) {
            set_frame(frame);
            used_frames_++;
        }
    }

    print_stats();
}

PhysicalAddress PhysicalAllocator::allocate_frame() {
    usize frame = find_free_frame();
    if (frame == static_cast<usize>(-1)) {
        kernel::panic("Out of physical memory!");
    }

    set_frame(frame);
    used_frames_++;

    return frame * FRAME_SIZE;
}

void PhysicalAllocator::free_frame(PhysicalAddress addr) {
    usize frame = addr / FRAME_SIZE;
    if (frame >= total_frames_) {
        drivers::serial_printf("WARNING: Attempt to free invalid frame: 0x%lx\n", addr);
        return;
    }

    if (!test_frame(frame)) {
        drivers::serial_printf("WARNING: Double free of frame: 0x%lx\n", addr);
        return;
    }

    clear_frame(frame);
    used_frames_--;
}

PhysicalAddress PhysicalAllocator::allocate_frames(usize count) {
    usize start_frame = find_free_frames(count);
    if (start_frame == static_cast<usize>(-1)) {
        kernel::panic("Out of contiguous physical memory!");
    }

    for (usize i = 0; i < count; i++) {
        set_frame(start_frame + i);
        used_frames_++;
    }

    return start_frame * FRAME_SIZE;
}

void PhysicalAllocator::free_frames(PhysicalAddress addr, usize count) {
    for (usize i = 0; i < count; i++) {
        free_frame(addr + i * FRAME_SIZE);
    }
}

usize PhysicalAllocator::total_frames() {
    return total_frames_;
}

usize PhysicalAllocator::used_frames() {
    return used_frames_;
}

usize PhysicalAllocator::free_frames() {
    return total_frames_ - used_frames_;
}

void PhysicalAllocator::print_stats() {
    drivers::kprintf("\n=== Physical Memory Statistics ===\n");
    drivers::kprintf("Total frames: %u (%u MB)\n",
                    total_frames_,
                    (total_frames_ * FRAME_SIZE) / (1024 * 1024));
    drivers::kprintf("Used frames:  %u (%u MB)\n",
                    used_frames_,
                    (used_frames_ * FRAME_SIZE) / (1024 * 1024));
    drivers::kprintf("Free frames:  %u (%u MB)\n",
                    free_frames(),
                    (free_frames() * FRAME_SIZE) / (1024 * 1024));

    drivers::serial_printf("Physical memory: %lu MB free / %lu MB total\n",
                          (free_frames() * FRAME_SIZE) / (1024 * 1024),
                          (total_frames_ * FRAME_SIZE) / (1024 * 1024));
}

void PhysicalAllocator::set_frame(usize frame_index) {
    usize idx = frame_index / BITMAP_ENTRIES_PER_UINT32;
    usize bit = frame_index % BITMAP_ENTRIES_PER_UINT32;
    bitmap_[idx] |= (1U << bit);
}

void PhysicalAllocator::clear_frame(usize frame_index) {
    usize idx = frame_index / BITMAP_ENTRIES_PER_UINT32;
    usize bit = frame_index % BITMAP_ENTRIES_PER_UINT32;
    bitmap_[idx] &= ~(1U << bit);
}

bool PhysicalAllocator::test_frame(usize frame_index) {
    usize idx = frame_index / BITMAP_ENTRIES_PER_UINT32;
    usize bit = frame_index % BITMAP_ENTRIES_PER_UINT32;
    return (bitmap_[idx] & (1U << bit)) != 0;
}

usize PhysicalAllocator::find_free_frame() {
    for (usize i = 0; i < bitmap_size_; i++) {
        if (bitmap_[i] != 0xFFFFFFFF) {
            // Found a uint32 with at least one free bit
            for (usize bit = 0; bit < BITMAP_ENTRIES_PER_UINT32; bit++) {
                if ((bitmap_[i] & (1U << bit)) == 0) {
                    return i * BITMAP_ENTRIES_PER_UINT32 + bit;
                }
            }
        }
    }
    return static_cast<usize>(-1);
}

usize PhysicalAllocator::find_free_frames(usize count) {
    usize found = 0;
    usize start_frame = 0;

    for (usize frame = 0; frame < total_frames_; frame++) {
        if (!test_frame(frame)) {
            if (found == 0) {
                start_frame = frame;
            }
            found++;
            if (found == count) {
                return start_frame;
            }
        } else {
            found = 0;
        }
    }

    return static_cast<usize>(-1);
}

} // namespace tiny_os::memory
