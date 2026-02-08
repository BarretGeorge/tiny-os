#!/bin/bash
# Create bootable ISO image for tiny-os

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
ISO_DIR="${BUILD_DIR}/iso"

echo "=== Creating bootable ISO image ==="

# Create ISO directory structure
mkdir -p "${ISO_DIR}/boot/grub"

# Copy kernel
cp "${BUILD_DIR}/kernel.elf" "${ISO_DIR}/boot/kernel.elf"

# Create GRUB configuration
cat > "${ISO_DIR}/boot/grub/grub.cfg" << 'EOF'
set timeout=0
set default=0

menuentry "tiny-os" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

# Create ISO using grub-mkrescue
echo "Creating ISO with grub-mkrescue..."
grub-mkrescue -o "${BUILD_DIR}/tiny-os.iso" "${ISO_DIR}"

echo "=== ISO created: ${BUILD_DIR}/tiny-os.iso ==="
ls -lh "${BUILD_DIR}/tiny-os.iso"
