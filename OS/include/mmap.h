#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>
#include <stddef.h>

// Memory protection flags
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

// Memory mapping flags
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x04
#define MAP_ANONYMOUS 0x08

// Memory mapping structure
typedef struct {
    void* addr;           // Mapped address
    size_t length;        // Length of mapping
    int prot;            // Protection flags
    int flags;           // Mapping flags
    int fd;              // File descriptor
    off_t offset;        // Offset into file
    uint32_t refcount;   // Reference count
    struct page* pages;  // List of mapped pages
} mmap_region_t;

// Memory mapping operations
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t length);
int mprotect(void* addr, size_t len, int prot);
int msync(void* addr, size_t length, int flags);
int mlock(const void* addr, size_t len);
int munlock(const void* addr, size_t len);

// Memory advice
#define MADV_NORMAL     0
#define MADV_RANDOM     1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED   3
#define MADV_DONTNEED   4

int madvise(void* addr, size_t length, int advice);

// Memory mapping information
struct mmap_info {
    void* addr;
    size_t length;
    int prot;
    int flags;
    off_t offset;
    char pathname[256];
};

int mmap_get_info(void* addr, struct mmap_info* info);

#endif /* MMAP_H */
