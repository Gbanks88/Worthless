#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>

// File types
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_DEVICE
} file_type_t;

// File permissions
typedef struct {
    uint16_t read: 1;
    uint16_t write: 1;
    uint16_t execute: 1;
    uint16_t user_read: 1;
    uint16_t user_write: 1;
    uint16_t user_execute: 1;
    uint16_t group_read: 1;
    uint16_t group_write: 1;
    uint16_t group_execute: 1;
} file_permissions_t;

// File attributes
typedef struct {
    file_type_t type;
    file_permissions_t permissions;
    uint32_t size;
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint32_t owner_id;
    uint32_t group_id;
} file_attributes_t;

// File descriptor
typedef struct {
    uint32_t id;
    char* path;
    uint32_t position;
    file_attributes_t attributes;
} file_descriptor_t;

// Filesystem operations
typedef struct {
    // Mount/unmount operations
    int (*mount)(const char* device, const char* mountpoint);
    int (*unmount)(const char* mountpoint);
    
    // File operations
    file_descriptor_t* (*open)(const char* path, int flags);
    int (*close)(file_descriptor_t* fd);
    size_t (*read)(file_descriptor_t* fd, void* buffer, size_t size);
    size_t (*write)(file_descriptor_t* fd, const void* buffer, size_t size);
    int (*seek)(file_descriptor_t* fd, int offset, int whence);
    
    // Directory operations
    int (*mkdir)(const char* path, file_permissions_t permissions);
    int (*rmdir)(const char* path);
    int (*readdir)(const char* path, char*** entries, size_t* count);
    
    // File management
    int (*create)(const char* path, file_permissions_t permissions);
    int (*remove)(const char* path);
    int (*rename)(const char* oldpath, const char* newpath);
    int (*chmod)(const char* path, file_permissions_t permissions);
    int (*chown)(const char* path, uint32_t owner, uint32_t group);
    int (*stat)(const char* path, file_attributes_t* attributes);
} filesystem_ops_t;

// Filesystem registration
int register_filesystem(const char* name, filesystem_ops_t* ops);
int unregister_filesystem(const char* name);

// High-level filesystem API
file_descriptor_t* fs_open(const char* path, int flags);
int fs_close(file_descriptor_t* fd);
size_t fs_read(file_descriptor_t* fd, void* buffer, size_t size);
size_t fs_write(file_descriptor_t* fd, const void* buffer, size_t size);
int fs_seek(file_descriptor_t* fd, int offset, int whence);
int fs_mkdir(const char* path, file_permissions_t permissions);
int fs_rmdir(const char* path);
int fs_remove(const char* path);
int fs_rename(const char* oldpath, const char* newpath);
int fs_stat(const char* path, file_attributes_t* attributes);

#endif /* FILESYSTEM_H */
