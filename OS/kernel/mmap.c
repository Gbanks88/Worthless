#include "../include/mmap.h"
#include "../include/memory.h"
#include "../include/filesystem.h"
#include <string.h>

#define MAX_MMAP_REGIONS 1024
#define PAGE_SIZE 4096

// Memory mapping table
static mmap_region_t* mmap_regions[MAX_MMAP_REGIONS];

// Page structure
struct page {
    void* vaddr;         // Virtual address
    uint64_t paddr;      // Physical address
    uint32_t flags;      // Page flags
    uint32_t refcount;   // Reference count
    struct page* next;   // Next page in list
};

// Create new memory mapping
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (length == 0) return (void*)-1;
    
    // Round up length to page size
    size_t num_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_size = num_pages * PAGE_SIZE;
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (!mmap_regions[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return (void*)-1;
    
    // Allocate region structure
    mmap_region_t* region = kmalloc(sizeof(mmap_region_t));
    if (!region) return (void*)-1;
    
    // Allocate virtual address space
    void* mapped_addr;
    if (addr && (flags & MAP_FIXED)) {
        mapped_addr = addr;
    } else {
        mapped_addr = vmalloc(total_size);
    }
    if (!mapped_addr) {
        kfree(region);
        return (void*)-1;
    }
    
    // Initialize region
    region->addr = mapped_addr;
    region->length = length;
    region->prot = prot;
    region->flags = flags;
    region->fd = fd;
    region->offset = offset;
    region->refcount = 1;
    region->pages = NULL;
    
    // Allocate and map pages
    struct page* last_page = NULL;
    for (size_t i = 0; i < num_pages; i++) {
        struct page* page = kmalloc(sizeof(struct page));
        if (!page) {
            // Cleanup on failure
            munmap(mapped_addr, length);
            return (void*)-1;
        }
        
        // Allocate physical page
        page->paddr = (uint64_t)pmalloc(PAGE_SIZE);
        if (!page->paddr) {
            kfree(page);
            munmap(mapped_addr, length);
            return (void*)-1;
        }
        
        page->vaddr = mapped_addr + (i * PAGE_SIZE);
        page->flags = prot;
        page->refcount = 1;
        page->next = NULL;
        
        // Map page
        if (map_page(page->vaddr, page->paddr, prot) < 0) {
            pfree((void*)page->paddr);
            kfree(page);
            munmap(mapped_addr, length);
            return (void*)-1;
        }
        
        // Link page into list
        if (last_page) {
            last_page->next = page;
        } else {
            region->pages = page;
        }
        last_page = page;
        
        // If mapping a file, read its contents
        if (fd >= 0 && !(flags & MAP_ANONYMOUS)) {
            ssize_t bytes_read = pread(fd, page->vaddr, PAGE_SIZE, offset + (i * PAGE_SIZE));
            if (bytes_read < 0) {
                munmap(mapped_addr, length);
                return (void*)-1;
            }
        }
    }
    
    mmap_regions[slot] = region;
    return mapped_addr;
}

// Unmap memory region
int munmap(void* addr, size_t length) {
    if (!addr || length == 0) return -1;
    
    // Find region
    mmap_region_t* region = NULL;
    int slot = -1;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (mmap_regions[i] && mmap_regions[i]->addr == addr) {
            region = mmap_regions[i];
            slot = i;
            break;
        }
    }
    if (!region) return -1;
    
    // Sync changes to file if necessary
    if (region->fd >= 0 && !(region->flags & MAP_PRIVATE)) {
        msync(addr, length, MS_SYNC);
    }
    
    // Free pages
    struct page* page = region->pages;
    while (page) {
        struct page* next = page->next;
        
        // Unmap page
        unmap_page(page->vaddr);
        
        // Free physical memory
        pfree((void*)page->paddr);
        
        // Free page structure
        kfree(page);
        
        page = next;
    }
    
    // Free region
    kfree(region);
    mmap_regions[slot] = NULL;
    
    return 0;
}

// Change protection on memory region
int mprotect(void* addr, size_t len, int prot) {
    if (!addr || len == 0) return -1;
    
    // Find region
    mmap_region_t* region = NULL;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (mmap_regions[i] && mmap_regions[i]->addr <= addr &&
            (char*)mmap_regions[i]->addr + mmap_regions[i]->length >= (char*)addr + len) {
            region = mmap_regions[i];
            break;
        }
    }
    if (!region) return -1;
    
    // Update protection
    struct page* page = region->pages;
    while (page) {
        if (page->vaddr >= addr && (char*)page->vaddr < (char*)addr + len) {
            if (update_page_protection(page->vaddr, prot) < 0) {
                return -1;
            }
            page->flags = prot;
        }
        page = page->next;
    }
    
    return 0;
}

// Sync memory region with file
int msync(void* addr, size_t length, int flags) {
    if (!addr || length == 0) return -1;
    
    // Find region
    mmap_region_t* region = NULL;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (mmap_regions[i] && mmap_regions[i]->addr == addr) {
            region = mmap_regions[i];
            break;
        }
    }
    if (!region || region->fd < 0) return -1;
    
    // Write pages back to file
    struct page* page = region->pages;
    size_t offset = 0;
    
    while (page && offset < length) {
        ssize_t bytes_written = pwrite(region->fd, page->vaddr, PAGE_SIZE, region->offset + offset);
        if (bytes_written < 0) return -1;
        
        page = page->next;
        offset += PAGE_SIZE;
    }
    
    return 0;
}

// Lock memory region
int mlock(const void* addr, size_t len) {
    if (!addr || len == 0) return -1;
    
    // Find region
    mmap_region_t* region = NULL;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (mmap_regions[i] && mmap_regions[i]->addr <= addr &&
            (char*)mmap_regions[i]->addr + mmap_regions[i]->length >= (char*)addr + len) {
            region = mmap_regions[i];
            break;
        }
    }
    if (!region) return -1;
    
    // Lock pages
    struct page* page = region->pages;
    while (page) {
        if (page->vaddr >= addr && (char*)page->vaddr < (char*)addr + len) {
            if (lock_page(page->vaddr) < 0) {
                return -1;
            }
        }
        page = page->next;
    }
    
    return 0;
}

// Get memory mapping information
int mmap_get_info(void* addr, struct mmap_info* info) {
    if (!addr || !info) return -1;
    
    // Find region
    mmap_region_t* region = NULL;
    for (int i = 0; i < MAX_MMAP_REGIONS; i++) {
        if (mmap_regions[i] && mmap_regions[i]->addr == addr) {
            region = mmap_regions[i];
            break;
        }
    }
    if (!region) return -1;
    
    // Fill info structure
    info->addr = region->addr;
    info->length = region->length;
    info->prot = region->prot;
    info->flags = region->flags;
    info->offset = region->offset;
    
    // Get pathname if file-backed
    if (region->fd >= 0) {
        get_file_path(region->fd, info->pathname, sizeof(info->pathname));
    } else {
        strcpy(info->pathname, "[anonymous]");
    }
    
    return 0;
}
