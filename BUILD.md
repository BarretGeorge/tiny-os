# Build Instructions for tiny-os

This document provides detailed instructions for setting up the development environment and building tiny-os.

## Prerequisites

### Required Tools

1. **Cross-Compiler Toolchain** (x86_64-elf-gcc/g++)
2. **NASM** (Netwide Assembler) 2.15+
3. **CMake** 3.20+
4. **Make** or Ninja
5. **GRUB2** tools (grub-mkrescue, xorriso)
6. **QEMU** (qemu-system-x86_64)
7. **GDB** (optional, for debugging)

### Platform-Specific Instructions

#### Ubuntu/Debian

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake nasm

# Install GRUB tools
sudo apt install grub-pc-bin grub-common xorriso mtools

# Install QEMU
sudo apt install qemu-system-x86

# Install GDB (optional)
sudo apt install gdb
```

**Building x86_64-elf Cross-Compiler:**

```bash
# Install dependencies
sudo apt install libgmp-dev libmpfr-dev libmpc-dev texinfo

# Set up directories
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p ~/src
cd ~/src

# Download binutils and gcc
wget https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz

# Build binutils
tar -xzf binutils-2.40.tar.gz
cd binutils-2.40
mkdir build && cd build
../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install

# Build gcc
cd ~/src
tar -xzf gcc-13.2.0.tar.gz
cd gcc-13.2.0
mkdir build && cd build
../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc

# Add to PATH permanently
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify installation
x86_64-elf-gcc --version
x86_64-elf-g++ --version
```

#### Arch Linux

```bash
# Install required packages
sudo pacman -S base-devel cmake nasm grub xorriso qemu-system-x86 gdb

# Cross-compiler is available in AUR
yay -S x86_64-elf-gcc x86_64-elf-binutils
# or
paru -S x86_64-elf-gcc x86_64-elf-binutils
```

#### macOS

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install tools
brew install cmake nasm qemu i686-elf-gcc i686-elf-binutils

# For cross-compiler, you may need to build from source (similar to Ubuntu instructions)
# or use a pre-built toolchain
```

#### Windows (WSL2)

We recommend using WSL2 with Ubuntu. Follow the Ubuntu instructions above after setting up WSL2.

```powershell
# Install WSL2
wsl --install

# Launch Ubuntu
wsl

# Then follow Ubuntu instructions
```

## Building tiny-os

### Method 1: Using Build Script (Recommended)

```bash
cd /path/to/tiny-os
./scripts/build.sh
```

This script will:
1. Create a `build/` directory
2. Run CMake configuration
3. Compile the kernel
4. Create a bootable ISO image

### Method 2: Manual Build

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build kernel
make -j$(nproc)

# Create ISO (after kernel is built)
cd ..
./scripts/create-iso.sh
```

### Build Outputs

After a successful build, you'll find:

- `build/kernel.elf` - Kernel executable (ELF format)
- `build/tiny-os.iso` - Bootable ISO image
- `build/*.o` - Object files
- `build/CMakeFiles/` - CMake build files

## Running tiny-os

### Running in QEMU

```bash
./scripts/run-qemu.sh
```

**QEMU Options:**
- `-cdrom tiny-os.iso` - Boot from ISO
- `-m 512M` - Allocate 512MB RAM
- `-serial stdio` - Redirect serial output to terminal
- `-no-reboot` - Exit instead of rebooting on triple fault
- `-no-shutdown` - Keep QEMU open on halt

**Exit QEMU:**
- Press `Ctrl+A` then `X`
- Or: `Ctrl+C` in the terminal

### Debugging with GDB

**Terminal 1:** Start QEMU with GDB stub

```bash
./scripts/debug-qemu.sh
```

This starts QEMU with:
- `-s` - Enable GDB server on port 1234
- `-S` - Pause execution at startup

**Terminal 2:** Connect GDB

```bash
gdb build/kernel.elf

# In GDB prompt:
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
(gdb) step
(gdb) info registers
(gdb) x/10i $rip
```

**Useful GDB Commands:**
- `break <function>` - Set breakpoint
- `continue` - Resume execution
- `step` / `stepi` - Step through code
- `next` / `nexti` - Step over function calls
- `info registers` - Show CPU registers
- `x/10i $rip` - Disassemble at current instruction
- `bt` - Backtrace (call stack)

## Common Build Issues

### Issue: `x86_64-elf-gcc: command not found`

**Solution:** Install or build the cross-compiler toolchain. Make sure `$HOME/opt/cross/bin` is in your PATH.

```bash
export PATH="$HOME/opt/cross/bin:$PATH"
# Or add permanently to ~/.bashrc
```

### Issue: `grub-mkrescue: command not found`

**Solution:** Install GRUB tools.

```bash
# Ubuntu/Debian
sudo apt install grub-pc-bin grub-common xorriso

# Arch
sudo pacman -S grub xorriso
```

### Issue: CMake can't find compilers

**Solution:** Specify compilers explicitly.

```bash
cmake .. -DCMAKE_C_COMPILER=x86_64-elf-gcc \
         -DCMAKE_CXX_COMPILER=x86_64-elf-g++ \
         -DCMAKE_ASM_NASM_COMPILER=nasm
```

### Issue: Linker errors about missing libgcc

**Solution:** Ensure libgcc is built for the target.

```bash
# When building gcc cross-compiler:
make all-target-libgcc
make install-target-libgcc
```

### Issue: QEMU doesn't start

**Solution:** Check QEMU installation and ISO path.

```bash
# Test QEMU
qemu-system-x86_64 --version

# Check ISO exists
ls -lh build/tiny-os.iso
```

### Issue: Kernel triple faults immediately

**Solution:** This indicates a bug in early boot code. Use QEMU's debugging features:

```bash
qemu-system-x86_64 -cdrom build/tiny-os.iso -d int,cpu_reset -D qemu.log
cat qemu.log
```

## Clean Build

To start fresh:

```bash
# Remove build directory
rm -rf build/

# Rebuild
./scripts/build.sh
```

## Advanced Build Options

### Release Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Verbose Build

```bash
make VERBOSE=1
```

### Custom Toolchain

Create a custom toolchain file:

```cmake
# custom-toolchain.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER /path/to/x86_64-elf-gcc)
set(CMAKE_CXX_COMPILER /path/to/x86_64-elf-g++)
set(CMAKE_ASM_NASM_COMPILER /path/to/nasm)
```

Then use it:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=custom-toolchain.cmake
```

## Testing on Real Hardware

**⚠️ Warning:** Running on real hardware can be risky. Only do this if you know what you're doing.

### Using USB Drive

```bash
# Write ISO to USB (replace /dev/sdX with your USB device)
sudo dd if=build/tiny-os.iso of=/dev/sdX bs=4M status=progress
sudo sync

# Boot from USB
# Configure BIOS/UEFI to boot from USB
```

### Using VirtualBox

1. Create a new VM with type "Other/Unknown (64-bit)"
2. Attach `build/tiny-os.iso` as optical drive
3. Boot the VM

## Continuous Integration

For automated builds, use the following CI configuration:

**.github/workflows/build.yml** (example)

```yaml
name: Build tiny-os

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y nasm grub-pc-bin grub-common xorriso qemu-system-x86
      - name: Build cross-compiler
        run: ./scripts/install-cross-compiler.sh
      - name: Build kernel
        run: ./scripts/build.sh
      - name: Test boot
        run: timeout 30 ./scripts/run-qemu.sh || true
```

## Next Steps

After successfully building tiny-os:

1. Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system design
2. Explore the source code in `src/` and `include/`
3. Try modifying the kernel and rebuilding
4. Contribute to the project!

## Getting Help

If you encounter issues not covered here:

1. Check the [OSDev Wiki](https://wiki.osdev.org/)
2. Review QEMU logs (`-D qemu.log`)
3. Enable verbose build output (`make VERBOSE=1`)
4. Open an issue on GitHub with:
   - Your OS and version
   - Compiler versions
   - Full error message
   - Build commands used

---

**Happy OS Development!** 🚀
