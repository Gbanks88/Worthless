#include "../include/sysv_ipc.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_SIZE 4096
#define TEST_KEY 12345

// Test data patterns
static const uint8_t pattern_a[8] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
static const uint8_t pattern_b[8] = {0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F};

// Test callback for monitoring
void test_monitor_callback(const char* event, void* arg) {
    printf("Monitor event: %s\n", event);
}

// Test encryption
void test_encryption(int shmid) {
    printf("\nTesting encryption...\n");
    
    uint8_t key[32];
    for (int i = 0; i < 32; i++) key[i] = i;
    
    if (shm_set_security(shmid, SEC_ENCRYPT, key, sizeof(key)) != 0) {
        printf("Failed to set encryption\n");
        return;
    }
    
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        printf("Failed to attach encrypted segment\n");
        return;
    }
    
    // Write pattern
    memcpy(addr, pattern_a, sizeof(pattern_a));
    
    // Detach (triggers encryption)
    shmdt(addr);
    
    // Reattach (triggers decryption)
    addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        printf("Failed to reattach encrypted segment\n");
        return;
    }
    
    // Verify pattern
    if (memcmp(addr, pattern_a, sizeof(pattern_a)) == 0) {
        printf("Encryption test passed\n");
    } else {
        printf("Encryption test failed\n");
    }
    
    shmdt(addr);
}

// Test integrity
void test_integrity(int shmid) {
    printf("\nTesting integrity...\n");
    
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        printf("Failed to attach segment\n");
        return;
    }
    
    // Write pattern
    memcpy(addr, pattern_b, sizeof(pattern_b));
    
    // Verify integrity
    if (shm_verify_integrity(shmid) == 0) {
        printf("Integrity verification passed\n");
    } else {
        printf("Integrity verification failed\n");
    }
    
    // Corrupt memory
    ((uint8_t*)addr)[0] ^= 0xFF;
    
    // Verify integrity again
    if (shm_verify_integrity(shmid) != 0) {
        printf("Corruption detection passed\n");
    } else {
        printf("Corruption detection failed\n");
    }
    
    shmdt(addr);
}

// Test monitoring
void test_monitoring(int shmid) {
    printf("\nTesting monitoring...\n");
    
    if (shm_monitor_segment(shmid, test_monitor_callback, NULL) != 0) {
        printf("Failed to set monitor\n");
        return;
    }
    
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        printf("Failed to attach segment\n");
        return;
    }
    
    // Perform some operations to trigger monitoring
    memcpy(addr, pattern_a, sizeof(pattern_a));
    memcpy(addr, pattern_b, sizeof(pattern_b));
    
    shmdt(addr);
}

// Main test function
int main() {
    printf("Starting IPC tests on Raspberry Pi 3...\n");
    
    // Create shared memory segment
    int shmid = shmget(TEST_KEY, TEST_SIZE, IPC_CREAT | SHM_R | SHM_W);
    if (shmid < 0) {
        printf("Failed to create shared memory segment\n");
        return 1;
    }
    
    // Run tests
    test_encryption(shmid);
    test_integrity(shmid);
    test_monitoring(shmid);
    
    // Get statistics
    struct shm_info info;
    if (shm_get_stats(&info) == 0) {
        printf("\nShared Memory Statistics:\n");
        printf("Used segments: %u\n", info.used_ids);
        printf("Total memory: %lu bytes\n", info.shm_tot);
        printf("Resident memory: %lu bytes\n", info.shm_rss);
        printf("Swapped memory: %u bytes\n", info.shm_swp);
    }
    
    printf("\nTests completed.\n");
    return 0;
}
