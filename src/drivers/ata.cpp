#include <tiny_os/drivers/ata.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::drivers {

// Static member definitions
ATADevice* ATAManager::devices_[MAX_DEVICES];

ATADevice::ATADevice(BusType bus, DriveType drive)
    : exists_(false), total_sectors_(0) {

    // Set I/O ports based on bus
    if (bus == BusType::PRIMARY) {
        io_base_ = PRIMARY_IO;
        ctrl_base_ = PRIMARY_CTRL;
    } else {
        io_base_ = SECONDARY_IO;
        ctrl_base_ = SECONDARY_CTRL;
    }

    // Set drive select value
    drive_select_value_ = (drive == DriveType::MASTER) ? 0xA0 : 0xB0;

    memset(model_, 0, sizeof(model_));
    memset(serial_, 0, sizeof(serial_));
}

ATADevice::~ATADevice() {
}

bool ATADevice::init() {
    serial_printf("[ATA] Initializing device at IO=0x%x, drive=0x%x\n",
                  io_base_, drive_select_value_);

    // Try to identify the device
    if (!identify()) {
        serial_printf("[ATA] No device found\n");
        return false;
    }

    exists_ = true;
    serial_printf("[ATA] Device found: %s (%d sectors, %d MB)\n",
                  model_, total_sectors_,
                  (total_sectors_ * 512) / (1024 * 1024));

    return true;
}

bool ATADevice::read_sectors(uint64 lba, usize count, void* buffer) {
    if (!exists_ || lba >= (1ULL << 28)) {
        return false;  // LBA28 limitation
    }

    uint8* buf = static_cast<uint8*>(buffer);

    for (usize i = 0; i < count; i++) {
        // Select drive
        select_drive();

        // Wait for drive to be ready
        if (!wait_ready()) {
            serial_printf("[ATA] Read: Drive not ready\n");
            return false;
        }

        // Set sector count and LBA
        outb(io_base_ + SECTOR_COUNT, 1);
        outb(io_base_ + LBA_LOW, (lba + i) & 0xFF);
        outb(io_base_ + LBA_MID, ((lba + i) >> 8) & 0xFF);
        outb(io_base_ + LBA_HIGH, ((lba + i) >> 16) & 0xFF);
        outb(io_base_ + DRIVE_SELECT,
             drive_select_value_ | (((lba + i) >> 24) & 0x0F));

        // Send READ command
        outb(io_base_ + COMMAND, READ_SECTORS);

        // Wait for data
        if (!wait_drq()) {
            serial_printf("[ATA] Read: Data not ready\n");
            return false;
        }

        // Read data (256 words = 512 bytes)
        read_pio(buf + i * 512, 256);
    }

    return true;
}

bool ATADevice::write_sectors(uint64 lba, usize count, const void* buffer) {
    if (!exists_ || lba >= (1ULL << 28)) {
        return false;  // LBA28 limitation
    }

    const uint8* buf = static_cast<const uint8*>(buffer);

    for (usize i = 0; i < count; i++) {
        // Select drive
        select_drive();

        // Wait for drive to be ready
        if (!wait_ready()) {
            serial_printf("[ATA] Write: Drive not ready\n");
            return false;
        }

        // Set sector count and LBA
        outb(io_base_ + SECTOR_COUNT, 1);
        outb(io_base_ + LBA_LOW, (lba + i) & 0xFF);
        outb(io_base_ + LBA_MID, ((lba + i) >> 8) & 0xFF);
        outb(io_base_ + LBA_HIGH, ((lba + i) >> 16) & 0xFF);
        outb(io_base_ + DRIVE_SELECT,
             drive_select_value_ | (((lba + i) >> 24) & 0x0F));

        // Send WRITE command
        outb(io_base_ + COMMAND, WRITE_SECTORS);

        // Wait for ready to accept data
        if (!wait_drq()) {
            serial_printf("[ATA] Write: Not ready for data\n");
            return false;
        }

        // Write data (256 words = 512 bytes)
        write_pio(buf + i * 512, 256);

        // Wait for write to complete
        if (!wait_ready()) {
            serial_printf("[ATA] Write: Completion failed\n");
            return false;
        }
    }

    return true;
}

void ATADevice::select_drive() {
    outb(io_base_ + DRIVE_SELECT, drive_select_value_);
    // Small delay after drive selection
    for (int i = 0; i < 4; i++) {
        inb(ctrl_base_);
    }
}

bool ATADevice::wait_ready() {
    for (int i = 0; i < 10000; i++) {
        uint8 status = inb(io_base_ + STATUS);
        if ((status & BSY) == 0 && (status & RDY) != 0) {
            return true;
        }
        // Small delay
        for (int j = 0; j < 100; j++) {
            asm volatile("pause");
        }
    }
    return false;
}

bool ATADevice::wait_drq() {
    for (int i = 0; i < 10000; i++) {
        uint8 status = inb(io_base_ + STATUS);
        if ((status & BSY) == 0 && (status & DRQ) != 0) {
            return true;
        }
        if (status & ERR) {
            serial_printf("[ATA] Error bit set (status=0x%x)\n", status);
            return false;
        }
        // Small delay
        for (int j = 0; j < 100; j++) {
            asm volatile("pause");
        }
    }
    return false;
}

void ATADevice::read_pio(void* buffer, usize words) {
    uint16* buf = static_cast<uint16*>(buffer);
    for (usize i = 0; i < words; i++) {
        buf[i] = inw(io_base_ + DATA);
    }
}

void ATADevice::write_pio(const void* buffer, usize words) {
    const uint16* buf = static_cast<const uint16*>(buffer);
    for (usize i = 0; i < words; i++) {
        outw(io_base_ + DATA, buf[i]);
    }
}

void ATADevice::software_reset() {
    outb(ctrl_base_, 0x04);  // Set SRST bit
    for (int i = 0; i < 100; i++) {
        inb(ctrl_base_);
    }
    outb(ctrl_base_, 0x00);  // Clear SRST bit
}

bool ATADevice::identify() {
    // Select drive
    select_drive();

    // Disable interrupts
    outb(ctrl_base_, 0x02);

    // Send IDENTIFY command
    outb(io_base_ + COMMAND, IDENTIFY);

    // Check if device exists
    uint8 status = inb(io_base_ + STATUS);
    if (status == 0) {
        return false;  // No device
    }

    // Wait for BSY to clear
    if (!wait_ready()) {
        return false;
    }

    // Check for ATAPI device (we don't support these yet)
    uint8 lba_mid = inb(io_base_ + LBA_MID);
    uint8 lba_high = inb(io_base_ + LBA_HIGH);
    if (lba_mid != 0 || lba_high != 0) {
        serial_printf("[ATA] ATAPI device detected (not supported)\n");
        return false;
    }

    // Wait for data
    if (!wait_drq()) {
        return false;
    }

    // Read IDENTIFY data (256 words)
    uint16 identify_data[256];
    read_pio(identify_data, 256);

    // Parse the data
    parse_identify_data(identify_data);

    return true;
}

void ATADevice::parse_identify_data(const uint16* data) {
    // Total sectors (words 60-61 for LBA28)
    total_sectors_ = data[60] | (static_cast<uint64>(data[61]) << 16);

    // Model string (words 27-46, byte-swapped)
    for (int i = 0; i < 20; i++) {
        uint16 word = data[27 + i];
        model_[i * 2] = (word >> 8) & 0xFF;
        model_[i * 2 + 1] = word & 0xFF;
    }
    model_[40] = '\0';

    // Trim trailing spaces
    for (int i = 39; i >= 0 && model_[i] == ' '; i--) {
        model_[i] = '\0';
    }

    // Serial number (words 10-19, byte-swapped)
    for (int i = 0; i < 10; i++) {
        uint16 word = data[10 + i];
        serial_[i * 2] = (word >> 8) & 0xFF;
        serial_[i * 2 + 1] = word & 0xFF;
    }
    serial_[20] = '\0';

    // Trim trailing spaces
    for (int i = 19; i >= 0 && serial_[i] == ' '; i--) {
        serial_[i] = '\0';
    }
}

// ATAManager implementation
void ATAManager::init() {
    serial_printf("[ATA] Initializing ATA manager\n");

    for (usize i = 0; i < MAX_DEVICES; i++) {
        devices_[i] = nullptr;
    }

    serial_printf("[ATA] ATA manager initialized\n");
    kprintf("[ATA] ATA manager initialized\n");
}

void ATAManager::detect_devices() {
    serial_printf("[ATA] Detecting ATA devices...\n");
    kprintf("[ATA] Detecting ATA devices...\n");

    // Try all 4 possible ATA devices
    ATADevice* dev0 = new ATADevice(ATADevice::BusType::PRIMARY, ATADevice::DriveType::MASTER);
    ATADevice* dev1 = new ATADevice(ATADevice::BusType::PRIMARY, ATADevice::DriveType::SLAVE);
    ATADevice* dev2 = new ATADevice(ATADevice::BusType::SECONDARY, ATADevice::DriveType::MASTER);
    ATADevice* dev3 = new ATADevice(ATADevice::BusType::SECONDARY, ATADevice::DriveType::SLAVE);

    if (dev0->init()) {
        devices_[0] = dev0;
        kprintf("  [0] Primary Master: %s\n", dev0->get_model());
    } else {
        delete dev0;
    }

    if (dev1->init()) {
        devices_[1] = dev1;
        kprintf("  [1] Primary Slave: %s\n", dev1->get_model());
    } else {
        delete dev1;
    }

    if (dev2->init()) {
        devices_[2] = dev2;
        kprintf("  [2] Secondary Master: %s\n", dev2->get_model());
    } else {
        delete dev2;
    }

    if (dev3->init()) {
        devices_[3] = dev3;
        kprintf("  [3] Secondary Slave: %s\n", dev3->get_model());
    } else {
        delete dev3;
    }

    serial_printf("[ATA] Device detection complete\n");
}

ATADevice* ATAManager::get_device(usize index) {
    if (index < MAX_DEVICES) {
        return devices_[index];
    }
    return nullptr;
}

ATADevice* ATAManager::get_primary_master() {
    return devices_[0];
}

} // namespace tiny_os::drivers
