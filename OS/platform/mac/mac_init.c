#include "../../include/driver.h"
#include "../../include/memory.h"

// Mac-specific hardware abstraction layer
static hal_interface_t mac_hal = {
    .init_memory = mac_init_memory,
    .init_interrupts = mac_init_interrupts,
    .init_devices = mac_init_devices
};

// Initialize Mac platform
void init_mac_platform(void) {
    // Set up Mac-specific memory management
    init_mac_memory();
    
    // Initialize Mac interrupt handlers
    init_mac_interrupts();
    
    // Set up Mac device drivers
    init_mac_drivers();
    
    // Register HAL interface
    register_hal_interface(&mac_hal);
}

// Mac-specific memory initialization
void mac_init_memory(void) {
    // Set up Mac virtual memory mapping
    init_paging();
    
    // Configure Mac-specific memory protection
    setup_mac_memory_protection();
}

// Mac interrupt initialization
void mac_init_interrupts(void) {
    // Set up Mac interrupt descriptor table
    setup_mac_idt();
    
    // Configure Mac interrupt controller
    init_mac_interrupt_controller();
    
    // Set up Mac exception handlers
    setup_mac_exception_handlers();
}

// Initialize Mac devices
void mac_init_devices(void) {
    // Initialize basic Mac devices
    init_mac_timer();
    init_mac_keyboard();
    init_mac_display();
    
    // Initialize Mac-specific hardware
    init_mac_specific_hardware();
}

// Initialize Mac drivers
void init_mac_drivers(void) {
    // Register Mac-specific drivers
    register_driver(&mac_keyboard_driver);
    register_driver(&mac_display_driver);
    register_driver(&mac_timer_driver);
    register_driver(&mac_storage_driver);
}
