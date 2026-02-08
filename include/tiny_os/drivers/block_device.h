#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::drivers {

// Block device interface (for disks, etc.)
class BlockDevice {
public:
    virtual ~BlockDevice() = default;

    // Read sectors from device
    // lba: Logical Block Address
    // count: Number of sectors to read
    // buffer: Buffer to read into
    // Returns: true on success, false on error
    virtual bool read_sectors(uint64 lba, usize count, void* buffer) = 0;

    // Write sectors to device
    virtual bool write_sectors(uint64 lba, usize count, const void* buffer) = 0;

    // Get sector size in bytes (usually 512)
    virtual usize sector_size() const = 0;

    // Get total number of sectors
    virtual uint64 total_sectors() const = 0;

    // Flush any cached writes
    virtual bool flush() { return true; }

    // Device information
    virtual const char* get_model() const = 0;
    virtual const char* get_serial() const = 0;
};

} // namespace tiny_os::drivers
