#include <tiny_os/memory/heap_allocator.h>
#include <tiny_os/common/string.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/kernel/kernel.h>

namespace tiny_os::memory {

HeapBlockHeader* HeapAllocator::free_list_head_ = nullptr;
VirtualAddress HeapAllocator::heap_start_ = 0;
usize HeapAllocator::heap_size_ = 0;
usize HeapAllocator::used_bytes_ = 0;

void HeapAllocator::init(VirtualAddress start, usize size) {
    drivers::kprintf("\nInitializing kernel heap...\n");
    drivers::serial_printf("Heap init: start=0x%lx, size=%lu bytes\n", start, size);

    heap_start_ = start;
    heap_size_ = size;
    used_bytes_ = 0;

    // Initialize free list with one large block
    free_list_head_ = reinterpret_cast<HeapBlockHeader*>(start);
    free_list_head_->size = size;
    free_list_head_->is_free = true;
    free_list_head_->next = nullptr;
    free_list_head_->magic = HeapBlockHeader::MAGIC_VALUE;

    drivers::kprintf("Heap initialized: %u MB at 0x%x\n",
                    size / (1024 * 1024), start);
    drivers::serial_printf("Heap ready\n");
}

void* HeapAllocator::kmalloc(usize size) {
    if (size == 0) return nullptr;

    // Align size to 16 bytes
    size = (size + 15) & ~15;

    // Find suitable free block
    HeapBlockHeader* block = find_free_block(size + sizeof(HeapBlockHeader));
    if (!block) {
        drivers::serial_printf("ERROR: kmalloc failed, size=%lu\n", size);
        return nullptr;
    }

    // Split block if it's much larger than needed
    split_block(block, size + sizeof(HeapBlockHeader));

    // Mark block as used
    block->is_free = false;
    used_bytes_ += block->size;

    // Return pointer to data (after header)
    return reinterpret_cast<void*>(
        reinterpret_cast<uint8*>(block) + sizeof(HeapBlockHeader));
}

void HeapAllocator::kfree(void* ptr) {
    if (!ptr) return;

    // Get block header
    auto* block = reinterpret_cast<HeapBlockHeader*>(
        reinterpret_cast<uint8*>(ptr) - sizeof(HeapBlockHeader));

    // Verify magic number
    if (block->magic != HeapBlockHeader::MAGIC_VALUE) {
        drivers::serial_printf("ERROR: Heap corruption detected at 0x%lx\n",
                              reinterpret_cast<uint64>(ptr));
        kernel::panic("Heap corruption!");
    }

    if (block->is_free) {
        drivers::serial_printf("WARNING: Double free at 0x%lx\n",
                              reinterpret_cast<uint64>(ptr));
        return;
    }

    // Mark block as free
    block->is_free = true;
    used_bytes_ -= block->size;

    // Merge with adjacent free blocks
    merge_free_blocks();
}

void* HeapAllocator::kmalloc_aligned(usize size, usize alignment) {
    // Allocate extra space for alignment
    void* ptr = kmalloc(size + alignment + sizeof(usize));
    if (!ptr) return nullptr;

    // Calculate aligned address
    usize addr = reinterpret_cast<usize>(ptr);
    usize aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    // Store original pointer before aligned address
    if (aligned_addr == addr) {
        aligned_addr += alignment;
    }
    *reinterpret_cast<usize*>(aligned_addr - sizeof(usize)) = addr;

    return reinterpret_cast<void*>(aligned_addr);
}

usize HeapAllocator::total_size() {
    return heap_size_;
}

usize HeapAllocator::used_size() {
    return used_bytes_;
}

usize HeapAllocator::free_size() {
    return heap_size_ - used_bytes_;
}

void HeapAllocator::print_stats() {
    drivers::kprintf("\n=== Heap Statistics ===\n");
    drivers::kprintf("Total size: %u KB\n", heap_size_ / 1024);
    drivers::kprintf("Used:       %u KB\n", used_bytes_ / 1024);
    drivers::kprintf("Free:       %u KB\n", free_size() / 1024);
    drivers::kprintf("Usage:      %u%%\n",
                    (used_bytes_ * 100) / heap_size_);

    drivers::serial_printf("Heap: %lu KB used / %lu KB total\n",
                          used_bytes_ / 1024,
                          heap_size_ / 1024);
}

void HeapAllocator::merge_free_blocks() {
    HeapBlockHeader* current = free_list_head_;

    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            // Check if blocks are adjacent
            uint8* current_end = reinterpret_cast<uint8*>(current) + current->size;
            if (current_end == reinterpret_cast<uint8*>(current->next)) {
                // Merge blocks
                current->size += current->next->size;
                current->next = current->next->next;
                continue;  // Check again with same block
            }
        }
        current = current->next;
    }
}

HeapBlockHeader* HeapAllocator::find_free_block(usize size) {
    HeapBlockHeader* current = free_list_head_;

    while (current) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return nullptr;
}

void HeapAllocator::split_block(HeapBlockHeader* block, usize size) {
    if (block->size < size + MIN_BLOCK_SIZE) {
        // Block is not large enough to split
        return;
    }

    // Create new free block from remainder
    auto* new_block = reinterpret_cast<HeapBlockHeader*>(
        reinterpret_cast<uint8*>(block) + size);

    new_block->size = block->size - size;
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->magic = HeapBlockHeader::MAGIC_VALUE;

    block->size = size;
    block->next = new_block;
}

} // namespace tiny_os::memory

// Update global operator new/delete to use real heap allocator
void* operator new(usize size) {
    if (tiny_os::memory::HeapAllocator::total_size() > 0) {
        return tiny_os::memory::HeapAllocator::kmalloc(size);
    } else {
        // Fall back to early allocator if heap not initialized
        extern void* tiny_os_early_malloc(usize);
        return tiny_os_early_malloc(size);
    }
}

void* operator new[](usize size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    if (tiny_os::memory::HeapAllocator::total_size() > 0) {
        tiny_os::memory::HeapAllocator::kfree(ptr);
    }
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete(void* ptr, usize) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, usize) noexcept {
    operator delete(ptr);
}
