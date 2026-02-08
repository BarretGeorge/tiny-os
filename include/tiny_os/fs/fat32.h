#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/fs/vfs.h>

namespace tiny_os::fs {

// Forward declaration
class BlockDevice;

// FAT32 Boot Sector (BIOS Parameter Block)
struct FAT32BootSector {
    uint8 jump[3];                  // Jump instruction
    char oem_name[8];               // OEM name
    uint16 bytes_per_sector;        // Bytes per sector (usually 512)
    uint8 sectors_per_cluster;      // Sectors per cluster
    uint16 reserved_sectors;        // Reserved sectors
    uint8 num_fats;                 // Number of FATs (usually 2)
    uint16 root_entries;            // Root directory entries (0 for FAT32)
    uint16 total_sectors_16;        // Total sectors (for small volumes)
    uint8 media_type;               // Media type
    uint16 fat_size_16;             // FAT size (0 for FAT32)
    uint16 sectors_per_track;       // Sectors per track
    uint16 num_heads;               // Number of heads
    uint32 hidden_sectors;          // Hidden sectors
    uint32 total_sectors_32;        // Total sectors (for large volumes)

    // FAT32-specific fields
    uint32 fat_size_32;             // Sectors per FAT
    uint16 ext_flags;               // Extended flags
    uint16 fs_version;              // Filesystem version
    uint32 root_cluster;            // Root directory cluster (usually 2)
    uint16 fs_info;                 // FSInfo sector number
    uint16 backup_boot_sector;      // Backup boot sector
    uint8 reserved[12];             // Reserved
    uint8 drive_number;             // Drive number
    uint8 reserved1;                // Reserved
    uint8 boot_signature;           // Extended boot signature (0x29)
    uint32 volume_id;               // Volume serial number
    char volume_label[11];          // Volume label
    char fs_type[8];                // Filesystem type ("FAT32   ")
} __attribute__((packed));

static_assert(sizeof(FAT32BootSector) == 90, "FAT32BootSector size must be 90 bytes");

// FAT32 Directory Entry (32 bytes)
struct FAT32DirEntry {
    char name[8];                   // Filename (8.3 format)
    char ext[3];                    // Extension
    uint8 attr;                     // Attributes
    uint8 reserved;                 // Reserved (NT)
    uint8 create_time_tenth;        // Creation time (tenths of second)
    uint16 create_time;             // Creation time
    uint16 create_date;             // Creation date
    uint16 access_date;             // Last access date
    uint16 cluster_high;            // High 16 bits of cluster
    uint16 modify_time;             // Modification time
    uint16 modify_date;             // Modification date
    uint16 cluster_low;             // Low 16 bits of cluster
    uint32 file_size;               // File size
} __attribute__((packed));

static_assert(sizeof(FAT32DirEntry) == 32, "FAT32DirEntry size must be 32 bytes");

// Directory entry attributes
namespace FAT32Attr {
    constexpr uint8 READ_ONLY = 0x01;
    constexpr uint8 HIDDEN = 0x02;
    constexpr uint8 SYSTEM = 0x04;
    constexpr uint8 VOLUME_ID = 0x08;
    constexpr uint8 DIRECTORY = 0x10;
    constexpr uint8 ARCHIVE = 0x20;
    constexpr uint8 LONG_NAME = 0x0F;  // LFN entry
}

// Special cluster values
namespace FAT32Cluster {
    constexpr uint32 FREE = 0x00000000;
    constexpr uint32 RESERVED = 0x00000001;
    constexpr uint32 BAD = 0x0FFFFFF7;
    constexpr uint32 EOC = 0x0FFFFFF8;  // End of chain (any value >= this)
}

// Long File Name entry
struct FAT32LFNEntry {
    uint8 order;                    // Order/sequence number
    uint16 name1[5];                // First 5 characters (Unicode)
    uint8 attr;                     // Attributes (always 0x0F)
    uint8 type;                     // Type (always 0)
    uint8 checksum;                 // Checksum of short name
    uint16 name2[6];                // Next 6 characters
    uint16 zero;                    // Always 0
    uint16 name3[2];                // Last 2 characters
} __attribute__((packed));

// FAT32-specific inode data
struct FAT32InodeData {
    uint32 first_cluster;           // First cluster of file/directory
    uint32 dir_cluster;             // Parent directory cluster
    uint32 dir_index;               // Index in parent directory
};

// FAT32 Filesystem implementation
class FAT32 : public Filesystem {
public:
    // Mount a FAT32 filesystem from a block device
    static FAT32* mount(BlockDevice* device);

    ~FAT32() override;

    // Filesystem interface implementation
    File* open(const char* path, uint32 flags) override;
    void close(File* file) override;
    isize read(File* file, void* buffer, usize count) override;
    isize write(File* file, const void* buffer, usize count) override;
    uint64 seek(File* file, int64 offset, SeekWhence whence) override;

    Inode* lookup(Inode* dir, const char* name) override;
    int readdir(File* dir, DirectoryEntry* entry, usize index) override;
    int mkdir(Inode* parent, const char* name, uint32 mode) override;
    int rmdir(Inode* parent, const char* name) override;

    int create(Inode* parent, const char* name, uint32 mode) override;
    int unlink(Inode* parent, const char* name) override;
    int rename(Inode* old_dir, const char* old_name,
              Inode* new_dir, const char* new_name) override;

    const char* get_name() const override { return "FAT32"; }
    usize get_total_space() const override;
    usize get_free_space() const override;

private:
    FAT32(BlockDevice* device);

    bool init();
    bool read_boot_sector();
    bool read_fat();

    // Cluster operations
    uint32 get_next_cluster(uint32 cluster);
    uint32 allocate_cluster();
    void free_cluster_chain(uint32 start_cluster);
    bool read_cluster(uint32 cluster, void* buffer);
    bool write_cluster(uint32 cluster, const void* buffer);

    // FAT operations
    void set_fat_entry(uint32 cluster, uint32 value);
    bool flush_fat();

    // Directory operations
    FAT32DirEntry* find_dir_entry(uint32 dir_cluster, const char* name);
    bool read_dir_entry(uint32 dir_cluster, usize index, FAT32DirEntry* entry);
    bool write_dir_entry(uint32 dir_cluster, usize index, const FAT32DirEntry* entry);
    uint32 create_dir_entry(uint32 parent_cluster, const char* name, uint8 attr);

    // Name conversion
    void name_to_83(const char* name, char* name83);
    void name_from_83(const char* name83, char* name);

    // Helper functions
    uint32 cluster_to_lba(uint32 cluster);
    Inode* create_inode_from_entry(const FAT32DirEntry* entry, uint32 dir_cluster, uint32 index);

    BlockDevice* device_;
    FAT32BootSector boot_sector_;
    uint32* fat_;                   // Cached FAT table
    bool fat_dirty_;                // FAT needs to be written back

    uint32 root_cluster_;
    uint32 cluster_size_;           // Cluster size in bytes
    uint32 fat_start_lba_;          // LBA of first FAT
    uint32 data_start_lba_;         // LBA of data area
    uint32 total_clusters_;
};

} // namespace tiny_os::fs
