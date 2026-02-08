#include <tiny_os/fs/fat32.h>
#include <tiny_os/drivers/block_device.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/memory/heap_allocator.h>
#include <tiny_os/common/string.h>

namespace tiny_os::fs {

FAT32* FAT32::mount(BlockDevice* device) {
    if (!device) return nullptr;

    serial_printf("[FAT32] Attempting to mount FAT32 filesystem\n");

    FAT32* fs = new FAT32(device);
    if (!fs) {
        serial_printf("[FAT32] Failed to allocate FAT32 object\n");
        return nullptr;
    }

    if (!fs->init()) {
        serial_printf("[FAT32] Failed to initialize FAT32\n");
        delete fs;
        return nullptr;
    }

    serial_printf("[FAT32] Successfully mounted FAT32 filesystem\n");
    kprintf("[FAT32] Mounted FAT32 filesystem\n");
    kprintf("  Total space: %d MB\n", fs->get_total_space() / (1024 * 1024));
    kprintf("  Free space: %d MB\n", fs->get_free_space() / (1024 * 1024));

    return fs;
}

FAT32::FAT32(BlockDevice* device)
    : device_(device), fat_(nullptr), fat_dirty_(false) {
}

FAT32::~FAT32() {
    if (fat_) {
        if (fat_dirty_) {
            flush_fat();
        }
        delete[] fat_;
    }
}

bool FAT32::init() {
    // Read boot sector
    if (!read_boot_sector()) {
        return false;
    }

    // Read FAT table
    if (!read_fat()) {
        return false;
    }

    return true;
}

bool FAT32::read_boot_sector() {
    uint8 sector[512];

    if (!device_->read_sectors(0, 1, sector)) {
        serial_printf("[FAT32] Failed to read boot sector\n");
        return false;
    }

    // Copy boot sector
    memcpy(&boot_sector_, sector, sizeof(FAT32BootSector));

    // Validate signature
    if (boot_sector_.boot_signature != 0x29) {
        serial_printf("[FAT32] Invalid boot signature: 0x%x\n",
                     boot_sector_.boot_signature);
        return false;
    }

    // Check if it's FAT32
    if (strncmp(boot_sector_.fs_type, "FAT32", 5) != 0) {
        serial_printf("[FAT32] Not a FAT32 filesystem: %.8s\n",
                     boot_sector_.fs_type);
        return false;
    }

    // Calculate parameters
    root_cluster_ = boot_sector_.root_cluster;
    cluster_size_ = boot_sector_.bytes_per_sector * boot_sector_.sectors_per_cluster;
    fat_start_lba_ = boot_sector_.reserved_sectors;
    data_start_lba_ = boot_sector_.reserved_sectors +
                      (boot_sector_.num_fats * boot_sector_.fat_size_32);
    total_clusters_ = (boot_sector_.total_sectors_32 - data_start_lba_) /
                      boot_sector_.sectors_per_cluster;

    serial_printf("[FAT32] Boot sector parsed:\n");
    serial_printf("  Bytes per sector: %d\n", boot_sector_.bytes_per_sector);
    serial_printf("  Sectors per cluster: %d\n", boot_sector_.sectors_per_cluster);
    serial_printf("  Cluster size: %d bytes\n", cluster_size_);
    serial_printf("  Root cluster: %d\n", root_cluster_);
    serial_printf("  Total clusters: %d\n", total_clusters_);

    return true;
}

bool FAT32::read_fat() {
    // Allocate FAT table (in memory)
    usize fat_size = boot_sector_.fat_size_32 * boot_sector_.bytes_per_sector;
    fat_ = new uint32[fat_size / 4];
    if (!fat_) {
        serial_printf("[FAT32] Failed to allocate FAT table\n");
        return false;
    }

    // Read FAT from disk
    if (!device_->read_sectors(fat_start_lba_, boot_sector_.fat_size_32, fat_)) {
        serial_printf("[FAT32] Failed to read FAT table\n");
        delete[] fat_;
        fat_ = nullptr;
        return false;
    }

    serial_printf("[FAT32] FAT table loaded (%d sectors)\n", boot_sector_.fat_size_32);
    return true;
}

uint32 FAT32::get_next_cluster(uint32 cluster) {
    if (cluster < 2 || cluster >= total_clusters_) {
        return FAT32Cluster::EOC;
    }

    uint32 next = fat_[cluster] & 0x0FFFFFFF;  // Mask off top 4 bits

    if (next >= FAT32Cluster::EOC) {
        return FAT32Cluster::EOC;
    }

    return next;
}

uint32 FAT32::cluster_to_lba(uint32 cluster) {
    if (cluster < 2) return 0;
    return data_start_lba_ + (cluster - 2) * boot_sector_.sectors_per_cluster;
}

bool FAT32::read_cluster(uint32 cluster, void* buffer) {
    uint32 lba = cluster_to_lba(cluster);
    return device_->read_sectors(lba, boot_sector_.sectors_per_cluster, buffer);
}

bool FAT32::write_cluster(uint32 cluster, const void* buffer) {
    uint32 lba = cluster_to_lba(cluster);
    return device_->write_sectors(lba, boot_sector_.sectors_per_cluster, buffer);
}

File* FAT32::open(const char* path, uint32 flags) {
    serial_printf("[FAT32] Opening file: %s\n", path);

    // Start from root directory
    uint32 dir_cluster = root_cluster_;

    // Simple implementation: assume path is just a filename in root
    // TODO: implement full path traversal
    FAT32DirEntry* entry = find_dir_entry(dir_cluster, path);
    if (!entry) {
        serial_printf("[FAT32] File not found: %s\n", path);
        return nullptr;
    }

    // Create inode
    Inode* inode = create_inode_from_entry(entry, dir_cluster, 0);
    delete entry;

    if (!inode) {
        return nullptr;
    }

    // Create file
    File* file = new File();
    file->inode = inode;
    file->position = 0;
    file->flags = flags;
    file->ref_count = 1;
    file->fs = this;

    serial_printf("[FAT32] File opened: %s (size: %d bytes)\n",
                 path, inode->size);

    return file;
}

void FAT32::close(File* file) {
    if (!file) return;

    file->ref_count--;
    if (file->ref_count == 0) {
        if (file->inode) {
            if (file->inode->fs_specific) {
                delete static_cast<FAT32InodeData*>(file->inode->fs_specific);
            }
            delete file->inode;
        }
        delete file;
    }
}

isize FAT32::read(File* file, void* buffer, usize count) {
    if (!file || !file->inode) return -1;

    FAT32InodeData* data = static_cast<FAT32InodeData*>(file->inode->fs_specific);
    if (!data) return -1;

    // Don't read past end of file
    if (file->position >= file->inode->size) {
        return 0;  // EOF
    }

    if (file->position + count > file->inode->size) {
        count = file->inode->size - file->position;
    }

    uint8* buf = static_cast<uint8*>(buffer);
    usize bytes_read = 0;
    uint32 cluster = data->first_cluster;

    // Skip to cluster containing current position
    usize cluster_offset = file->position / cluster_size_;
    for (usize i = 0; i < cluster_offset && cluster < FAT32Cluster::EOC; i++) {
        cluster = get_next_cluster(cluster);
    }

    // Read data
    usize position_in_cluster = file->position % cluster_size_;

    while (count > 0 && cluster < FAT32Cluster::EOC) {
        // Read cluster
        uint8* cluster_data = new uint8[cluster_size_];
        if (!read_cluster(cluster, cluster_data)) {
            delete[] cluster_data;
            break;
        }

        // Copy requested bytes
        usize bytes_in_cluster = cluster_size_ - position_in_cluster;
        if (bytes_in_cluster > count) {
            bytes_in_cluster = count;
        }

        memcpy(buf + bytes_read, cluster_data + position_in_cluster, bytes_in_cluster);
        delete[] cluster_data;

        bytes_read += bytes_in_cluster;
        count -= bytes_in_cluster;
        position_in_cluster = 0;  // Next cluster starts from beginning

        // Move to next cluster
        cluster = get_next_cluster(cluster);
    }

    file->position += bytes_read;
    return bytes_read;
}

isize FAT32::write(File* file, const void* buffer, usize count) {
    // TODO: implement write support
    (void)file;
    (void)buffer;
    (void)count;
    return -1;  // Read-only for now
}

uint64 FAT32::seek(File* file, int64 offset, SeekWhence whence) {
    if (!file) return -1;

    uint64 new_pos;

    switch (whence) {
        case SeekWhence::SET:
            new_pos = offset;
            break;
        case SeekWhence::CUR:
            new_pos = file->position + offset;
            break;
        case SeekWhence::END:
            new_pos = file->inode->size + offset;
            break;
        default:
            return -1;
    }

    file->position = new_pos;
    return new_pos;
}

Inode* FAT32::lookup(Inode* dir, const char* name) {
    if (!dir || dir->type != FileType::DIRECTORY) {
        return nullptr;
    }

    FAT32InodeData* data = static_cast<FAT32InodeData*>(dir->fs_specific);
    if (!data) return nullptr;

    FAT32DirEntry* entry = find_dir_entry(data->first_cluster, name);
    if (!entry) return nullptr;

    Inode* inode = create_inode_from_entry(entry, data->first_cluster, 0);
    delete entry;

    return inode;
}

int FAT32::readdir(File* dir, DirectoryEntry* entry, usize index) {
    if (!dir || !dir->inode || dir->inode->type != FileType::DIRECTORY) {
        return -1;
    }

    FAT32InodeData* data = static_cast<FAT32InodeData*>(dir->inode->fs_specific);
    if (!data) return -1;

    FAT32DirEntry fat_entry;
    if (!read_dir_entry(data->first_cluster, index, &fat_entry)) {
        return -1;  // No more entries
    }

    // Skip deleted or empty entries
    if (fat_entry.name[0] == 0x00 || fat_entry.name[0] == 0xE5) {
        return -1;
    }

    // Convert to DirectoryEntry
    name_from_83(fat_entry.name, entry->name);

    if (fat_entry.attr & FAT32Attr::DIRECTORY) {
        entry->type = FileType::DIRECTORY;
    } else {
        entry->type = FileType::REGULAR;
    }

    entry->size = fat_entry.file_size;
    entry->inode_num = (fat_entry.cluster_high << 16) | fat_entry.cluster_low;

    return 0;
}

FAT32DirEntry* FAT32::find_dir_entry(uint32 dir_cluster, const char* name) {
    char name83[11];
    name_to_83(name, name83);

    uint8* cluster_data = new uint8[cluster_size_];
    uint32 cluster = dir_cluster;

    while (cluster < FAT32Cluster::EOC) {
        if (!read_cluster(cluster, cluster_data)) {
            break;
        }

        // Search directory entries in this cluster
        usize entries_per_cluster = cluster_size_ / sizeof(FAT32DirEntry);
        FAT32DirEntry* entries = reinterpret_cast<FAT32DirEntry*>(cluster_data);

        for (usize i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0x00) {
                // End of directory
                delete[] cluster_data;
                return nullptr;
            }

            if (entries[i].name[0] == 0xE5) {
                // Deleted entry, skip
                continue;
            }

            if (entries[i].attr == FAT32Attr::LONG_NAME) {
                // LFN entry, skip for now
                continue;
            }

            // Compare name
            if (memcmp(entries[i].name, name83, 11) == 0) {
                FAT32DirEntry* result = new FAT32DirEntry();
                *result = entries[i];
                delete[] cluster_data;
                return result;
            }
        }

        // Move to next cluster
        cluster = get_next_cluster(cluster);
    }

    delete[] cluster_data;
    return nullptr;
}

bool FAT32::read_dir_entry(uint32 dir_cluster, usize index, FAT32DirEntry* entry) {
    usize entries_per_cluster = cluster_size_ / sizeof(FAT32DirEntry);
    usize cluster_index = index / entries_per_cluster;
    usize entry_index = index % entries_per_cluster;

    // Find the cluster
    uint32 cluster = dir_cluster;
    for (usize i = 0; i < cluster_index && cluster < FAT32Cluster::EOC; i++) {
        cluster = get_next_cluster(cluster);
    }

    if (cluster >= FAT32Cluster::EOC) {
        return false;
    }

    // Read cluster
    uint8* cluster_data = new uint8[cluster_size_];
    if (!read_cluster(cluster, cluster_data)) {
        delete[] cluster_data;
        return false;
    }

    // Copy entry
    FAT32DirEntry* entries = reinterpret_cast<FAT32DirEntry*>(cluster_data);
    *entry = entries[entry_index];

    delete[] cluster_data;
    return true;
}

void FAT32::name_to_83(const char* name, char* name83) {
    // Convert filename to 8.3 format
    // Example: "test.txt" -> "TEST    TXT"

    memset(name83, ' ', 11);

    const char* dot = nullptr;
    for (const char* p = name; *p; p++) {
        if (*p == '.') dot = p;
    }

    // Copy name part (up to 8 characters)
    usize name_len = dot ? (dot - name) : strlen(name);
    if (name_len > 8) name_len = 8;

    for (usize i = 0; i < name_len; i++) {
        name83[i] = (name[i] >= 'a' && name[i] <= 'z') ?
                    (name[i] - 32) : name[i];  // Convert to uppercase
    }

    // Copy extension part (up to 3 characters)
    if (dot) {
        const char* ext = dot + 1;
        usize ext_len = strlen(ext);
        if (ext_len > 3) ext_len = 3;

        for (usize i = 0; i < ext_len; i++) {
            name83[8 + i] = (ext[i] >= 'a' && ext[i] <= 'z') ?
                            (ext[i] - 32) : ext[i];  // Convert to uppercase
        }
    }
}

void FAT32::name_from_83(const char* name83, char* name) {
    // Convert from 8.3 format to normal filename
    // Example: "TEST    TXT" -> "test.txt"

    usize pos = 0;

    // Copy name part (trim trailing spaces)
    for (usize i = 0; i < 8 && name83[i] != ' '; i++) {
        name[pos++] = (name83[i] >= 'A' && name83[i] <= 'Z') ?
                     (name83[i] + 32) : name83[i];  // Convert to lowercase
    }

    // Add extension if present
    if (name83[8] != ' ') {
        name[pos++] = '.';
        for (usize i = 8; i < 11 && name83[i] != ' '; i++) {
            name[pos++] = (name83[i] >= 'A' && name83[i] <= 'Z') ?
                         (name83[i] + 32) : name83[i];
        }
    }

    name[pos] = '\0';
}

Inode* FAT32::create_inode_from_entry(const FAT32DirEntry* entry,
                                     uint32 dir_cluster, uint32 index) {
    Inode* inode = new Inode();
    FAT32InodeData* data = new FAT32InodeData();

    data->first_cluster = (entry->cluster_high << 16) | entry->cluster_low;
    data->dir_cluster = dir_cluster;
    data->dir_index = index;

    inode->fs_specific = data;
    inode->fs = this;
    inode->inode_num = data->first_cluster;
    inode->size = entry->file_size;
    inode->type = (entry->attr & FAT32Attr::DIRECTORY) ?
                  FileType::DIRECTORY : FileType::REGULAR;
    inode->mode = 0644;  // Default permissions
    inode->uid = 0;
    inode->gid = 0;
    inode->link_count = 1;

    // TODO: convert FAT timestamps to Unix timestamps
    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;

    return inode;
}

int FAT32::mkdir(Inode* parent, const char* name, uint32 mode) {
    // TODO: implement
    (void)parent;
    (void)name;
    (void)mode;
    return -1;
}

int FAT32::rmdir(Inode* parent, const char* name) {
    // TODO: implement
    (void)parent;
    (void)name;
    return -1;
}

int FAT32::create(Inode* parent, const char* name, uint32 mode) {
    // TODO: implement
    (void)parent;
    (void)name;
    (void)mode;
    return -1;
}

int FAT32::unlink(Inode* parent, const char* name) {
    // TODO: implement
    (void)parent;
    (void)name;
    return -1;
}

int FAT32::rename(Inode* old_dir, const char* old_name,
                 Inode* new_dir, const char* new_name) {
    // TODO: implement
    (void)old_dir;
    (void)old_name;
    (void)new_dir;
    (void)new_name;
    return -1;
}

usize FAT32::get_total_space() const {
    return static_cast<usize>(total_clusters_) * cluster_size_;
}

usize FAT32::get_free_space() const {
    // Count free clusters
    usize free_clusters = 0;
    for (uint32 i = 2; i < total_clusters_; i++) {
        if ((fat_[i] & 0x0FFFFFFF) == FAT32Cluster::FREE) {
            free_clusters++;
        }
    }
    return free_clusters * cluster_size_;
}

uint32 FAT32::allocate_cluster() {
    // TODO: implement
    return 0;
}

void FAT32::free_cluster_chain(uint32 start_cluster) {
    // TODO: implement
    (void)start_cluster;
}

void FAT32::set_fat_entry(uint32 cluster, uint32 value) {
    if (cluster >= 2 && cluster < total_clusters_) {
        fat_[cluster] = (fat_[cluster] & 0xF0000000) | (value & 0x0FFFFFFF);
        fat_dirty_ = true;
    }
}

bool FAT32::flush_fat() {
    if (!fat_dirty_) return true;

    // Write FAT table back to disk
    if (!device_->write_sectors(fat_start_lba_, boot_sector_.fat_size_32, fat_)) {
        serial_printf("[FAT32] Failed to write FAT table\n");
        return false;
    }

    fat_dirty_ = false;
    return true;
}

bool FAT32::write_dir_entry(uint32 dir_cluster, usize index,
                            const FAT32DirEntry* entry) {
    // TODO: implement
    (void)dir_cluster;
    (void)index;
    (void)entry;
    return false;
}

uint32 FAT32::create_dir_entry(uint32 parent_cluster, const char* name,
                               uint8 attr) {
    // TODO: implement
    (void)parent_cluster;
    (void)name;
    (void)attr;
    return 0;
}

} // namespace tiny_os::fs
