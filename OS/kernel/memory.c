#include <stdint.h>
#include "../include/memory.h"

// Memory map entry from GRUB
struct memory_map_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

// Page directory entry
typedef struct {
    uint32_t present    : 1;
    uint32_t rw        : 1;
    uint32_t user      : 1;
    uint32_t accessed  : 1;
    uint32_t dirty     : 1;
    uint32_t unused    : 7;
    uint32_t frame     : 20;
} page_dir_entry_t;

// Page table entry
typedef struct {
    uint32_t present    : 1;
    uint32_t rw        : 1;
    uint32_t user      : 1;
    uint32_t accessed  : 1;
    uint32_t dirty     : 1;
    uint32_t unused    : 7;
    uint32_t frame     : 20;
} page_table_entry_t;

#define PAGE_SIZE 4096
#define PAGE_DIR_SIZE 1024
#define PAGE_TABLE_SIZE 1024

static page_dir_entry_t* page_directory = NULL;
static page_table_entry_t* page_tables = NULL;

// Initialize paging
void init_paging(void) {
    // Allocate page directory
    page_directory = (page_dir_entry_t*)alloc_frame();
    
    // Clear page directory
    for (int i = 0; i < PAGE_DIR_SIZE; i++) {
        page_directory[i].present = 0;
        page_directory[i].rw = 1;
        page_directory[i].user = 0;
        page_directory[i].frame = 0;
    }
    
    // Identity map first 4MB
    for (uint32_t i = 0; i < 1024; i++) {
        alloc_page(i * PAGE_SIZE, 0, 1);
    }
    
    // Enable paging
    enable_paging();
}

// Allocate a physical frame
void* alloc_frame(void) {
    static uint32_t next_frame = 0x100000; // Start after 1MB
    void* frame = (void*)next_frame;
    next_frame += PAGE_SIZE;
    return frame;
}

// Map virtual address to physical address
void map_page(uint32_t virtual_addr, uint32_t physical_addr, int user, int rw) {
    uint32_t pd_idx = virtual_addr >> 22;
    uint32_t pt_idx = (virtual_addr >> 12) & 0x3FF;
    
    // Create page table if it doesn't exist
    if (!page_directory[pd_idx].present) {
        page_table_entry_t* new_table = (page_table_entry_t*)alloc_frame();
        page_directory[pd_idx].frame = (uint32_t)new_table >> 12;
        page_directory[pd_idx].present = 1;
        page_directory[pd_idx].rw = rw;
        page_directory[pd_idx].user = user;
        
        // Clear new page table
        for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
            new_table[i].present = 0;
            new_table[i].rw = 0;
            new_table[i].user = 0;
            new_table[i].frame = 0;
        }
    }
    
    // Map the page
    page_table_entry_t* table = (page_table_entry_t*)(page_directory[pd_idx].frame << 12);
    table[pt_idx].frame = physical_addr >> 12;
    table[pt_idx].present = 1;
    table[pt_idx].rw = rw;
    table[pt_idx].user = user;
}

// Enable paging
void enable_paging(void) {
    // Load page directory
    asm volatile("mov %0, %%cr3":: "r"(page_directory));
    
    // Enable paging bit in cr0
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}
