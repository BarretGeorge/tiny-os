#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/drivers/block_device.h>

namespace tiny_os::drivers {

// ATA/IDE disk driver (PIO mode, LBA28)
class ATADevice : public BlockDevice {
public:
    // ATA bus types
    enum class BusType {
        PRIMARY,        // Primary bus (0x1F0)
        SECONDARY       // Secondary bus (0x170)
    };

    // Drive type
    enum class DriveType {
        MASTER,         // Master drive
        SLAVE           // Slave drive
    };

    // Create an ATA device
    ATADevice(BusType bus, DriveType drive);
    ~ATADevice() override;

    // Initialize and detect the device
    bool init();

    // BlockDevice interface
    bool read_sectors(uint64 lba, usize count, void* buffer) override;
    bool write_sectors(uint64 lba, usize count, const void* buffer) override;
    usize sector_size() const override { return 512; }
    uint64 total_sectors() const override { return total_sectors_; }

    const char* get_model() const override { return model_; }
    const char* get_serial() const override { return serial_; }

    // Check if device exists
    bool exists() const { return exists_; }

private:
    // ATA I/O ports
    static constexpr uint16 PRIMARY_IO = 0x1F0;
    static constexpr uint16 PRIMARY_CTRL = 0x3F6;
    static constexpr uint16 SECONDARY_IO = 0x170;
    static constexpr uint16 SECONDARY_CTRL = 0x376;

    // Port offsets
    enum Port {
        DATA = 0,
        ERROR = 1,
        SECTOR_COUNT = 2,
        LBA_LOW = 3,
        LBA_MID = 4,
        LBA_HIGH = 5,
        DRIVE_SELECT = 6,
        STATUS = 7,
        COMMAND = 7
    };

    // Status register bits
    enum Status {
        ERR = (1 << 0),     // Error
        DRQ = (1 << 3),     // Data request
        SRV = (1 << 4),     // Service
        DF = (1 << 5),      // Drive fault
        RDY = (1 << 6),     // Ready
        BSY = (1 << 7)      // Busy
    };

    // ATA commands
    enum Command {
        READ_SECTORS = 0x20,
        WRITE_SECTORS = 0x30,
        IDENTIFY = 0xEC
    };

    uint16 io_base_;
    uint16 ctrl_base_;
    uint8 drive_select_value_;
    bool exists_;
    uint64 total_sectors_;
    char model_[41];
    char serial_[21];

    // Low-level operations
    void select_drive();
    bool wait_ready();
    bool wait_drq();
    void read_pio(void* buffer, usize words);
    void write_pio(const void* buffer, usize words);
    void software_reset();

    // IDENTIFY command
    bool identify();
    void parse_identify_data(const uint16* data);
};

// ATA Manager - manages all ATA devices
class ATAManager {
public:
    static void init();

    // Detect all ATA devices
    static void detect_devices();

    // Get device by index (0-3)
    static ATADevice* get_device(usize index);

    // Get primary master (most common boot drive)
    static ATADevice* get_primary_master();

private:
    static constexpr usize MAX_DEVICES = 4;
    static ATADevice* devices_[MAX_DEVICES];
};

} // namespace tiny_os::drivers
