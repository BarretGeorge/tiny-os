#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os::fs {

// Forward declarations
struct Inode;
struct File;
class Filesystem;

// File types
enum class FileType {
    REGULAR,        // Regular file
    DIRECTORY,      // Directory
    SYMLINK,        // Symbolic link
    CHARDEV,        // Character device
    BLOCKDEV        // Block device
};

// File open flags (compatible with POSIX)
namespace OpenFlags {
    constexpr uint32 O_RDONLY = 0x0000;     // Read-only
    constexpr uint32 O_WRONLY = 0x0001;     // Write-only
    constexpr uint32 O_RDWR = 0x0002;       // Read-write
    constexpr uint32 O_CREAT = 0x0040;      // Create if not exists
    constexpr uint32 O_TRUNC = 0x0200;      // Truncate to 0 length
    constexpr uint32 O_APPEND = 0x0400;     // Append mode
}

// Seek positions
enum class SeekWhence {
    SET = 0,        // Absolute position
    CUR = 1,        // Current position + offset
    END = 2         // End of file + offset
};

// Directory entry
struct DirectoryEntry {
    char name[256];         // File name
    FileType type;          // File type
    usize size;             // File size
    uint64 inode_num;       // Inode number
};

// Inode (file metadata)
struct Inode {
    uint64 inode_num;       // Inode number
    FileType type;          // File type
    usize size;             // File size in bytes
    uint32 mode;            // Permissions (POSIX-style)
    uint32 uid;             // Owner user ID
    uint32 gid;             // Owner group ID
    uint64 atime;           // Access time
    uint64 mtime;           // Modification time
    uint64 ctime;           // Creation time
    uint32 link_count;      // Hard link count

    Filesystem* fs;         // Owning filesystem
    void* fs_specific;      // Filesystem-specific data
};

// Open file handle
struct File {
    Inode* inode;           // File inode
    uint64 position;        // Current read/write position
    uint32 flags;           // Open flags
    uint32 ref_count;       // Reference count

    Filesystem* fs;         // Filesystem operations
};

// Filesystem operations interface
class Filesystem {
public:
    virtual ~Filesystem() = default;

    // File operations
    virtual File* open(const char* path, uint32 flags) = 0;
    virtual void close(File* file) = 0;
    virtual isize read(File* file, void* buffer, usize count) = 0;
    virtual isize write(File* file, const void* buffer, usize count) = 0;
    virtual uint64 seek(File* file, int64 offset, SeekWhence whence) = 0;

    // Directory operations
    virtual Inode* lookup(Inode* dir, const char* name) = 0;
    virtual int readdir(File* dir, DirectoryEntry* entry, usize index) = 0;
    virtual int mkdir(Inode* parent, const char* name, uint32 mode) = 0;
    virtual int rmdir(Inode* parent, const char* name) = 0;

    // File management
    virtual int create(Inode* parent, const char* name, uint32 mode) = 0;
    virtual int unlink(Inode* parent, const char* name) = 0;
    virtual int rename(Inode* old_dir, const char* old_name,
                      Inode* new_dir, const char* new_name) = 0;

    // Filesystem info
    virtual const char* get_name() const = 0;
    virtual usize get_total_space() const = 0;
    virtual usize get_free_space() const = 0;
};

// Virtual File System manager
class VFS {
public:
    // Initialize VFS
    static void init();

    // Mount a filesystem at a path
    static int mount(const char* path, Filesystem* fs);

    // Unmount a filesystem
    static int unmount(const char* path);

    // File operations
    static File* open(const char* path, uint32 flags);
    static void close(File* file);
    static isize read(File* file, void* buffer, usize count);
    static isize write(File* file, const void* buffer, usize count);
    static uint64 seek(File* file, int64 offset, SeekWhence whence);

    // Directory operations
    static int readdir(File* dir, DirectoryEntry* entry, usize index);
    static int mkdir(const char* path, uint32 mode);
    static int rmdir(const char* path);

    // File management
    static int create(const char* path, uint32 mode);
    static int unlink(const char* path);
    static int rename(const char* old_path, const char* new_path);

    // Path resolution
    static Inode* resolve_path(const char* path);

private:
    static constexpr usize MAX_MOUNTS = 16;

    struct MountPoint {
        char path[256];
        Filesystem* fs;
        bool in_use;
    };

    static MountPoint mounts_[MAX_MOUNTS];
    static Filesystem* root_fs_;

    // Find filesystem for path
    static Filesystem* find_filesystem(const char* path, char* relative_path);

    // Path manipulation helpers
    static void normalize_path(const char* path, char* normalized);
    static bool split_path(const char* path, char* parent, char* name);
};

} // namespace tiny_os::fs
