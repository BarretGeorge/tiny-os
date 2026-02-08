#include <tiny_os/fs/vfs.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::fs {

// Static member definitions
VFS::MountPoint VFS::mounts_[MAX_MOUNTS];
Filesystem* VFS::root_fs_ = nullptr;

void VFS::init() {
    serial_printf("[VFS] Initializing Virtual File System...\n");

    // Clear all mount points
    for (usize i = 0; i < MAX_MOUNTS; i++) {
        mounts_[i].in_use = false;
        mounts_[i].fs = nullptr;
        mounts_[i].path[0] = '\0';
    }

    root_fs_ = nullptr;

    serial_printf("[VFS] VFS initialized\n");
    kprintf("[VFS] Virtual File System initialized\n");
}

int VFS::mount(const char* path, Filesystem* fs) {
    if (!fs) return -1;

    serial_printf("[VFS] Mounting %s at %s\n", fs->get_name(), path);

    // Special case: root mount
    if (strcmp(path, "/") == 0) {
        root_fs_ = fs;
        kprintf("[VFS] Mounted %s as root filesystem\n", fs->get_name());
        return 0;
    }

    // Find free mount point
    for (usize i = 0; i < MAX_MOUNTS; i++) {
        if (!mounts_[i].in_use) {
            mounts_[i].in_use = true;
            mounts_[i].fs = fs;

            usize len = strlen(path);
            if (len >= sizeof(mounts_[i].path)) len = sizeof(mounts_[i].path) - 1;
            memcpy(mounts_[i].path, path, len);
            mounts_[i].path[len] = '\0';

            kprintf("[VFS] Mounted %s at %s\n", fs->get_name(), path);
            return 0;
        }
    }

    serial_printf("[VFS] No free mount points!\n");
    return -1;
}

int VFS::unmount(const char* path) {
    if (strcmp(path, "/") == 0) {
        root_fs_ = nullptr;
        return 0;
    }

    for (usize i = 0; i < MAX_MOUNTS; i++) {
        if (mounts_[i].in_use && strcmp(mounts_[i].path, path) == 0) {
            mounts_[i].in_use = false;
            mounts_[i].fs = nullptr;
            return 0;
        }
    }

    return -1;
}

File* VFS::open(const char* path, uint32 flags) {
    char relative_path[256];
    Filesystem* fs = find_filesystem(path, relative_path);

    if (!fs) {
        serial_printf("[VFS] No filesystem for path: %s\n", path);
        return nullptr;
    }

    return fs->open(relative_path, flags);
}

void VFS::close(File* file) {
    if (!file || !file->fs) return;
    file->fs->close(file);
}

isize VFS::read(File* file, void* buffer, usize count) {
    if (!file || !file->fs) return -1;
    return file->fs->read(file, buffer, count);
}

isize VFS::write(File* file, const void* buffer, usize count) {
    if (!file || !file->fs) return -1;
    return file->fs->write(file, buffer, count);
}

uint64 VFS::seek(File* file, int64 offset, SeekWhence whence) {
    if (!file || !file->fs) return -1;
    return file->fs->seek(file, offset, whence);
}

int VFS::readdir(File* dir, DirectoryEntry* entry, usize index) {
    if (!dir || !dir->fs) return -1;
    return dir->fs->readdir(dir, entry, index);
}

int VFS::mkdir(const char* path, uint32 mode) {
    char relative_path[256];
    Filesystem* fs = find_filesystem(path, relative_path);

    if (!fs) return -1;

    // Split path into parent and name
    char parent[256], name[256];
    if (!split_path(relative_path, parent, name)) {
        return -1;
    }

    // TODO: resolve parent directory and call mkdir
    return -1;  // Not fully implemented
}

int VFS::rmdir(const char* path) {
    // TODO: implement
    (void)path;
    return -1;
}

int VFS::create(const char* path, uint32 mode) {
    // TODO: implement
    (void)path;
    (void)mode;
    return -1;
}

int VFS::unlink(const char* path) {
    // TODO: implement
    (void)path;
    return -1;
}

int VFS::rename(const char* old_path, const char* new_path) {
    // TODO: implement
    (void)old_path;
    (void)new_path;
    return -1;
}

Inode* VFS::resolve_path(const char* path) {
    // TODO: implement full path resolution
    (void)path;
    return nullptr;
}

Filesystem* VFS::find_filesystem(const char* path, char* relative_path) {
    // Normalize path
    char normalized[256];
    normalize_path(path, normalized);

    // Check mount points (longest match first)
    usize best_match_len = 0;
    Filesystem* best_match = nullptr;

    for (usize i = 0; i < MAX_MOUNTS; i++) {
        if (!mounts_[i].in_use) continue;

        usize len = strlen(mounts_[i].path);
        if (len > best_match_len &&
            strncmp(normalized, mounts_[i].path, len) == 0) {
            best_match_len = len;
            best_match = mounts_[i].fs;
        }
    }

    if (best_match) {
        // Copy relative path
        const char* rel = normalized + best_match_len;
        if (*rel == '/') rel++;  // Skip leading slash
        strcpy(relative_path, rel);
        return best_match;
    }

    // Try root filesystem
    if (root_fs_) {
        strcpy(relative_path, normalized);
        if (relative_path[0] == '/') {
            // Remove leading slash for relative path
            strcpy(relative_path, normalized + 1);
        }
        return root_fs_;
    }

    return nullptr;
}

void VFS::normalize_path(const char* path, char* normalized) {
    // Simple normalization: just copy for now
    // TODO: handle .., ., multiple slashes, etc.
    strcpy(normalized, path);
}

bool VFS::split_path(const char* path, char* parent, char* name) {
    // Find last slash
    const char* last_slash = nullptr;
    for (const char* p = path; *p; p++) {
        if (*p == '/') last_slash = p;
    }

    if (!last_slash) {
        // No slash, so parent is current dir
        strcpy(parent, ".");
        strcpy(name, path);
        return true;
    }

    // Copy parent (everything before last slash)
    usize parent_len = last_slash - path;
    if (parent_len == 0) {
        strcpy(parent, "/");
    } else {
        memcpy(parent, path, parent_len);
        parent[parent_len] = '\0';
    }

    // Copy name (everything after last slash)
    strcpy(name, last_slash + 1);

    return true;
}

} // namespace tiny_os::fs
