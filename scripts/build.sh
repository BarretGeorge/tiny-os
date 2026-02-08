#!/bin/bash
# Build script for tiny-os

set -e  # Exit on error

echo "=== Building tiny-os ==="

# Create build directory
mkdir -p build
cd build

# Run CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build kernel
echo "Building kernel..."
make -j$(nproc)

echo "=== Build complete ==="
ls -lh kernel.elf 2>/dev/null || echo "Note: kernel.elf will be built once source files are added"
