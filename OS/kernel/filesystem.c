#include "../include/filesystem.h"
#include "../include/memory.h"
#include <string.h>

#define MAX_FILESYSTEMS 16
#define MAX_OPEN_FILES 1024
#define MAX_PATH_LENGTH 4096

// Filesystem registry
static struct {
    char* name;
    filesystem_ops_t* ops;
} filesystem_registry[MAX_FILESYSTEMS];

static int num_filesystems = 0;

// Open file table
static file_descriptor_t* open_files[MAX_OPEN_FILES];
static uint32_t next_fd = 0;

// Register a filesystem
int register_filesystem(const char* name, filesystem_ops_t* ops) {
    if (!name || !ops || num_filesystems >= MAX_FILESYSTEMS) {
        return -1;
    }
    
    // Check for duplicate
    for (int i = 0; i < num_filesystems; i++) {
        if (strcmp(filesystem_registry[i].name, name) == 0) {
            return -1;
        }
    }
    
    // Add to registry
    filesystem_registry[num_filesystems].name = strdup(name);
    filesystem_registry[num_filesystems].ops = ops;
    num_filesystems++;
    
    return 0;
}

// Unregister a filesystem
int unregister_filesystem(const char* name) {
    if (!name) return -1;
    
    for (int i = 0; i < num_filesystems; i++) {
        if (strcmp(filesystem_registry[i].name, name) == 0) {
            kfree(filesystem_registry[i].name);
            
            // Shift remaining entries
            for (int j = i; j < num_filesystems - 1; j++) {
                filesystem_registry[j] = filesystem_registry[j + 1];
            }
            
            num_filesystems--;
            return 0;
        }
    }
    
    return -1;
}

// Find filesystem for path
static filesystem_ops_t* find_filesystem_for_path(const char* path) {
    // TODO: Implement mount point lookup
    // For now, return first registered filesystem
    if (num_filesystems > 0) {
        return filesystem_registry[0].ops;
    }
    return NULL;
}

// Open a file
file_descriptor_t* fs_open(const char* path, int flags) {
    if (!path) return NULL;
    
    filesystem_ops_t* fs = find_filesystem_for_path(path);
    if (!fs || !fs->open) return NULL;
    
    file_descriptor_t* fd = fs->open(path, flags);
    if (!fd) return NULL;
    
    // Add to open file table
    if (next_fd >= MAX_OPEN_FILES) {
        fs->close(fd);
        return NULL;
    }
    
    fd->id = next_fd++;
    open_files[fd->id] = fd;
    
    return fd;
}

// Close a file
int fs_close(file_descriptor_t* fd) {
    if (!fd || fd->id >= MAX_OPEN_FILES || !open_files[fd->id]) {
        return -1;
    }
    
    filesystem_ops_t* fs = find_filesystem_for_path(fd->path);
    if (!fs || !fs->close) return -1;
    
    int result = fs->close(fd);
    if (result == 0) {
        open_files[fd->id] = NULL;
    }
    
    return result;
}

// Read from file
size_t fs_read(file_descriptor_t* fd, void* buffer, size_t size) {
    if (!fd || !buffer || fd->id >= MAX_OPEN_FILES || !open_files[fd->id]) {
        return 0;
    }
    
    filesystem_ops_t* fs = find_filesystem_for_path(fd->path);
    if (!fs || !fs->read) return 0;
    
    return fs->read(fd, buffer, size);
}

// Write to file
size_t fs_write(file_descriptor_t* fd, const void* buffer, size_t size) {
    if (!fd || !buffer || fd->id >= MAX_OPEN_FILES || !open_files[fd->id]) {
        return 0;
    }
    
    filesystem_ops_t* fs = find_filesystem_for_path(fd->path);
    if (!fs || !fs->write) return 0;
    
    return fs->write(fd, buffer, size);
}

// Create directory
int fs_mkdir(const char* path, file_permissions_t permissions) {
    if (!path) return -1;
    
    filesystem_ops_t* fs = find_filesystem_for_path(path);
    if (!fs || !fs->mkdir) return -1;
    
    return fs->mkdir(path, permissions);
}

// Remove directory
int fs_rmdir(const char* path) {
    if (!path) return -1;
    
    filesystem_ops_t* fs = find_filesystem_for_path(path);
    if (!fs || !fs->rmdir) return -1;
    
    return fs->rmdir(path);
}

// Remove file
int fs_remove(const char* path) {
    if (!path) return -1;
    
    filesystem_ops_t* fs = find_filesystem_for_path(path);
    if (!fs || !fs->remove) return -1;
    
    return fs->remove(path);
}

// Rename file
int fs_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    
    filesystem_ops_t* fs = find_filesystem_for_path(oldpath);
    if (!fs || !fs->rename) return -1;
    
    return fs->rename(oldpath, newpath);
}

// Get file attributes
int fs_stat(const char* path, file_attributes_t* attributes) {
    if (!path || !attributes) return -1;
    
    filesystem_ops_t* fs = find_filesystem_for_path(path);
    if (!fs || !fs->stat) return -1;
    
    return fs->stat(path, attributes);
}
