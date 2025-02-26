#include <stddef.h>
#include <string.h>
#include "../include/driver.h"

#define MAX_DRIVERS 256

// Driver registry
static struct {
    driver_t* drivers[MAX_DRIVERS];
    int count;
} driver_registry = { .count = 0 };

// Register a new driver
int register_driver(driver_t* driver) {
    if (!driver || !driver->name || driver_registry.count >= MAX_DRIVERS) {
        return -1;
    }
    
    // Check for duplicate
    for (int i = 0; i < driver_registry.count; i++) {
        if (strcmp(driver_registry.drivers[i]->name, driver->name) == 0) {
            return -1;
        }
    }
    
    // Initialize the driver
    if (driver->init && driver->init() < 0) {
        return -1;
    }
    
    // Add to registry
    driver_registry.drivers[driver_registry.count++] = driver;
    return 0;
}

// Unregister a driver
int unregister_driver(const char* name) {
    if (!name) {
        return -1;
    }
    
    // Find and remove driver
    for (int i = 0; i < driver_registry.count; i++) {
        if (strcmp(driver_registry.drivers[i]->name, name) == 0) {
            // Cleanup driver
            if (driver_registry.drivers[i]->cleanup) {
                driver_registry.drivers[i]->cleanup();
            }
            
            // Remove from registry
            for (int j = i; j < driver_registry.count - 1; j++) {
                driver_registry.drivers[j] = driver_registry.drivers[j + 1];
            }
            driver_registry.count--;
            return 0;
        }
    }
    
    return -1;
}

// Find a driver by name
driver_t* find_driver(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; i < driver_registry.count; i++) {
        if (strcmp(driver_registry.drivers[i]->name, name) == 0) {
            return driver_registry.drivers[i];
        }
    }
    
    return NULL;
}

// Initialize platform-specific drivers
void init_platform_drivers(void) {
    #ifdef __APPLE__
        init_mac_drivers();
    #elif defined(__linux__)
        init_linux_drivers();
    #elif defined(_WIN32)
        init_windows_drivers();
    #endif
}
