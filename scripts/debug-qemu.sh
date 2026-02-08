#!/bin/bash
# Run tiny-os in QEMU with GDB stub for debugging

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
ISO_PATH="${BUILD_DIR}/tiny-os.iso"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"

if [ ! -f "${ISO_PATH}" ]; then
    echo "Error: ISO not found at ${ISO_PATH}"
    echo "Please run scripts/build.sh first"
    exit 1
fi

echo "=== Running tiny-os in QEMU with GDB stub ==="
echo "GDB server listening on localhost:1234"
echo ""
echo "In another terminal, run:"
echo "  gdb ${KERNEL_ELF}"
echo "  (gdb) target remote localhost:1234"
echo "  (gdb) break kernel_main"
echo "  (gdb) continue"
echo ""

qemu-system-x86_64 \
    -cdrom "${ISO_PATH}" \
    -m 512M \
    -serial stdio \
    -no-reboot \
    -no-shutdown \
    -s -S
