#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Memory management functions
void init_paging(void);
void* alloc_frame(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, int user, int rw);
void enable_paging(void);

// Memory allocation functions
void* kmalloc(size_t size);
void kfree(void* ptr);

// Memory constants
#define PAGE_SIZE 4096
#define KERNEL_VIRTUAL_BASE 0xC0000000
#define USER_VIRTUAL_BASE 0x40000000

#endif /* MEMORY_H */
