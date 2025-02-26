#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>

// Driver interface structure
typedef struct {
    const char* name;
    uint32_t version;
    
    // Driver operations
    int (*init)(void);
    int (*cleanup)(void);
    int (*read)(void* buffer, size_t size);
    int (*write)(const void* buffer, size_t size);
    int (*ioctl)(uint32_t cmd, void* arg);
} driver_t;

// Driver manager functions
int register_driver(driver_t* driver);
int unregister_driver(const char* name);
driver_t* find_driver(const char* name);

// Platform-specific driver initialization
void init_platform_drivers(void);

// Common driver commands
#define IOCTL_GET_VERSION    0x0001
#define IOCTL_RESET_DEVICE   0x0002
#define IOCTL_GET_STATUS     0x0003

#endif /* DRIVER_H */
