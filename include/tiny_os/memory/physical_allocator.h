#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::memory {

class PhysicalAllocator {
public:
    static void init(void* multiboot_info);

    // Allocate a 4KB physical frame
    static PhysicalAddress allocate_frame();

    // Free a 4KB physical frame
    static void free_frame(PhysicalAddress addr);

    // Allocate multiple contiguous frames
    static PhysicalAddress allocate_frames(usize count);

    // Free multiple contiguous frames
    static void free_frames(PhysicalAddress addr, usize count);

    // Statistics
    static usize total_frames();
    static usize used_frames();
    static usize free_frames();

    static void print_stats();

private:
    static constexpr usize FRAME_SIZE = 4096;
    static constexpr usize BITMAP_ENTRIES_PER_UINT32 = 32;

    static uint32* bitmap_;
    static usize bitmap_size_;  // In uint32s
    static usize total_frames_;
    static usize used_frames_;
    static PhysicalAddress memory_end_;

    // Kernel end symbol (defined in linker script)
    static uint8 kernel_end;

    static void set_frame(usize frame_index);
    static void clear_frame(usize frame_index);
    static bool test_frame(usize frame_index);
    static usize find_free_frame();
    static usize find_free_frames(usize count);
};

} // namespace tiny_os::memory
