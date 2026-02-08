#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::kernel {

// Kernel panic function
[[noreturn]] void panic(const char* message);

// Kernel main entry point (called from boot.asm)
extern "C" void kernel_main(uint32 magic, void* multiboot_info);

} // namespace tiny_os::kernel
