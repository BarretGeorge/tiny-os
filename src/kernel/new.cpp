#include <tiny_os/common/types.h>
#include <tiny_os/drivers/vga.h>

namespace tiny_os::kernel {

// Temporary heap for early boot (before real heap allocator is ready)
// This will be replaced with proper heap allocator in Phase 2
static uint8 early_heap[1024 * 1024];  // 1MB early heap
static usize early_heap_offset = 0;

void* early_malloc(usize size) {
    // Align to 16 bytes
    size = (size + 15) & ~15;

    if (early_heap_offset + size > sizeof(early_heap)) {
        drivers::kprintf("PANIC: Early heap exhausted!\n");
        while (true) {
            asm volatile("cli; hlt");
        }
    }

    void* ptr = &early_heap[early_heap_offset];
    early_heap_offset += size;
    return ptr;
}

} // namespace tiny_os::kernel

// Global operator new/delete implementations
void* operator new(usize size) {
    return tiny_os::kernel::early_malloc(size);
}

void* operator new[](usize size) {
    return tiny_os::kernel::early_malloc(size);
}

void operator delete(void* ptr) noexcept {
    // Early heap doesn't support freeing
    // This will be properly implemented in Phase 2
    (void)ptr;
}

void operator delete[](void* ptr) noexcept {
    (void)ptr;
}

void operator delete(void* ptr, usize size) noexcept {
    (void)ptr;
    (void)size;
}

void operator delete[](void* ptr, usize size) noexcept {
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
uintptr_t __stack_chk_guard = 0x595e9fbd94fda766;

void __stack_chk_fail() {
    tiny_os::drivers::kprintf("PANIC: Stack smashing detected!\n");
    while (true) {
        asm volatile("cli; hlt");
    }
}

} // extern "C"
