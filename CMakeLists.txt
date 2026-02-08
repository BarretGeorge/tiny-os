cmake_minimum_required(VERSION 3.20)
project(tiny-os VERSION 0.1.0 LANGUAGES C CXX ASM_NASM)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Toolchain configuration
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler prefix
if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER x86_64-elf-gcc)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER x86_64-elf-g++)
endif()
if(NOT DEFINED CMAKE_ASM_NASM_COMPILER)
    set(CMAKE_ASM_NASM_COMPILER nasm)
endif()

# Freestanding environment flags
set(COMMON_FLAGS "-ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -mcmodel=kernel")
set(COMMON_FLAGS "${COMMON_FLAGS} -fno-pic -fno-stack-protector -mno-sse -mno-sse2")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib -lgcc")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/boot/linker.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z max-page-size=0x1000")

# NASM flags
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS "-f elf64")

# Include directories
include_directories(include)

# Source files
set(KERNEL_SOURCES
    # Phase 1: Boot and basic kernel
    src/kernel/kernel_main.cpp
    src/kernel/new.cpp
    src/arch/x86_64/gdt.cpp
    src/drivers/vga.cpp
    src/drivers/serial.cpp
    src/common/string.cpp

    # Phase 2: Memory management
    # src/memory/physical_allocator.cpp
    # src/memory/virtual_allocator.cpp
    # src/memory/heap_allocator.cpp

    # Phase 3: Interrupt handling
    # src/arch/x86_64/idt.cpp
    # src/arch/x86_64/pic.cpp
    # src/drivers/timer.cpp

    # Phase 4: Process and thread management
    # src/process/process.cpp
    # src/process/thread.cpp
    # src/process/scheduler.cpp
    # src/process/syscall.cpp

    # Phase 5: Filesystem
    # src/fs/vfs.cpp
    # src/fs/fat32.cpp
    # src/drivers/ata.cpp

    # Phase 6: Drivers and shell
    # src/drivers/keyboard.cpp
    # src/kernel/shell.cpp
)

set(KERNEL_ASM_SOURCES
    # Phase 1: Boot code
    boot/boot.asm
    src/arch/x86_64/gdt.asm

    # Phase 3: Interrupt handling
    # src/arch/x86_64/idt.asm

    # Phase 4: Process management
    # src/process/context_switch.asm
    # src/process/syscall.asm
)

# Create kernel executable
add_executable(kernel.elf ${KERNEL_SOURCES} ${KERNEL_ASM_SOURCES})
set_target_properties(kernel.elf PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/boot/linker.ld)

# Custom target to create ISO
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/tiny-os.iso
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/create-iso.sh
    DEPENDS kernel.elf
    COMMENT "Creating bootable ISO image"
)

add_custom_target(iso ALL DEPENDS ${CMAKE_BINARY_DIR}/tiny-os.iso)

# QEMU targets
add_custom_target(run
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-qemu.sh
    DEPENDS iso
    COMMENT "Running tiny-os in QEMU"
)

add_custom_target(debug
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/debug-qemu.sh
    DEPENDS iso
    COMMENT "Running tiny-os in QEMU with GDB stub"
)

message(STATUS "tiny-os build system configured")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: Freestanding kernel")
