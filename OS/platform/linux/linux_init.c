#include "../../include/driver.h"
#include "../../include/memory.h"

// Linux-specific hardware abstraction layer
static hal_interface_t linux_hal = {
    .init_memory = linux_init_memory,
    .init_interrupts = linux_init_interrupts,
    .init_devices = linux_init_devices
};

// Initialize Linux platform
void init_linux_platform(void) {
    // Set up Linux-specific memory management
    init_linux_memory();
    
    // Initialize Linux interrupt handlers
    init_linux_interrupts();
    
    // Set up Linux device drivers
    init_linux_drivers();
    
    // Register HAL interface
    register_hal_interface(&linux_hal);
}

// Linux-specific memory initialization
void linux_init_memory(void) {
    // Set up Linux virtual memory mapping
    init_paging();
    
    // Configure Linux-specific memory protection
    setup_linux_memory_protection();
    
    // Initialize Linux-specific memory zones
    init_linux_memory_zones();
}

// Linux interrupt initialization
void linux_init_interrupts(void) {
    // Set up Linux interrupt descriptor table
    setup_linux_idt();
    
    // Configure Linux interrupt controller
    init_linux_interrupt_controller();
    
    // Set up Linux exception handlers
    setup_linux_exception_handlers();
    
    // Initialize syscall interface
    init_linux_syscalls();
}

// Initialize Linux devices
void linux_init_devices(void) {
    // Initialize basic Linux devices
    init_linux_timer();
    init_linux_keyboard();
    init_linux_display();
    
    // Initialize Linux-specific hardware
    init_linux_specific_hardware();
}

// Initialize Linux drivers
void init_linux_drivers(void) {
    // Register Linux-specific drivers
    register_driver(&linux_keyboard_driver);
    register_driver(&linux_display_driver);
    register_driver(&linux_timer_driver);
    register_driver(&linux_storage_driver);
    
    // Initialize Linux device filesystem
    init_linux_devfs();
}
