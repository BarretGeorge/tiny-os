#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::memory {

// Heap block header (placed before each allocated block)
struct HeapBlockHeader {
    usize size;              // Size including header
    bool is_free;
    HeapBlockHeader* next;
    uint32 magic;            // Magic number for corruption detection

    static constexpr uint32 MAGIC_VALUE = 0xDEADBEEF;
} __attribute__((packed));

class HeapAllocator {
public:
    static void init(VirtualAddress start, usize size);

    // Allocate memory
    static void* kmalloc(usize size);

    // Free memory
    static void kfree(void* ptr);

    // Allocate aligned memory
    static void* kmalloc_aligned(usize size, usize alignment);

    // Statistics
    static usize total_size();
    static usize used_size();
    static usize free_size();

    static void print_stats();

    // Merge adjacent free blocks
    static void merge_free_blocks();

private:
    static HeapBlockHeader* free_list_head_;
    static VirtualAddress heap_start_;
    static usize heap_size_;
    static usize used_bytes_;

    static constexpr usize MIN_BLOCK_SIZE = sizeof(HeapBlockHeader) + 16;

    // Find suitable free block (first-fit)
    static HeapBlockHeader* find_free_block(usize size);

    // Split block if it's too large
    static void split_block(HeapBlockHeader* block, usize size);
};

} // namespace tiny_os::memory
