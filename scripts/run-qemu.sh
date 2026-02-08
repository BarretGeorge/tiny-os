#!/bin/bash
# Run tiny-os in QEMU

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
ISO_PATH="${BUILD_DIR}/tiny-os.iso"

if [ ! -f "${ISO_PATH}" ]; then
    echo "Error: ISO not found at ${ISO_PATH}"
    echo "Please run scripts/build.sh first"
    exit 1
fi

echo "=== Running tiny-os in QEMU ==="
echo "Press Ctrl+A then X to exit QEMU"
echo ""

qemu-system-x86_64 \
    -cdrom "${ISO_PATH}" \
    -m 512M \
    -serial stdio \
    -no-reboot \
    -no-shutdown \
    -d int,cpu_reset
