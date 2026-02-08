#include <tiny_os/common/types.h>
#include <tiny_os/drivers/vga.h>
#include <cstddef>
#include <cstdint>

namespace tiny_os::kernel {

// Temporary heap for early boot (before real heap allocator is ready)
// This will be replaced with proper heap allocator in Phase 2
static uint8 early_heap[1024 * 1024];  // 1MB early heap
static usize early_heap_offset = 0;

} // namespace tiny_os::kernel

// Export for global operators
extern "C" void* tiny_os_early_malloc(size_t size) {
    using namespace tiny_os;

    // Align to 16 bytes
    size = (size + 15) & ~15;

    if (kernel::early_heap_offset + size > sizeof(kernel::early_heap)) {
        drivers::kprintf("PANIC: Early heap exhausted!\n");
        while (true) {
            asm volatile("cli; hlt");
        }
    }

    void* ptr = &kernel::early_heap[kernel::early_heap_offset];
    kernel::early_heap_offset += size;
    return ptr;
}

// Global operator new/delete implementations
void* operator new(size_t size) {
    return tiny_os_early_malloc(size);
}

void* operator new[](size_t size) {
    return tiny_os_early_malloc(size);
}

void operator delete(void* ptr) noexcept {
    // Early heap doesn't support freeing
    // This will be properly implemented in Phase 2
    (void)ptr;
}

void operator delete[](void* ptr) noexcept {
    (void)ptr;
}

void operator delete(void* ptr, size_t size) noexcept {
    (void)ptr;
    (void)size;
}

void operator delete[](void* ptr, size_t size) noexcept {
    (void)ptr;
    (void)size;
}

// Required C++ runtime support functions
extern "C" {

void __cxa_pure_virtual() {
    tiny_os::drivers::kprintf("PANIC: Pure virtual function called!\n");
    while (true) {
        asm volatile("cli; hlt");
    }
}

void __cxa_atexit(void (*)(void*), void*, void*) {
    // No-op for kernel (we never exit)
}

void __cxa_finalize(void*) {
    // No-op for kernel
}

// Stack protection (disabled but symbols needed)
uint64_t __stack_chk_guard = 0x595e9fbd94fda766;

void __stack_chk_fail() {
    tiny_os::drivers::kprintf("PANIC: Stack smashing detected!\n");
    while (true) {
        asm volatile("cli; hlt");
    }
}

} // extern "C"
