#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define TEST_SIZE 4096
#define TEST_KEY 12345

void test_basic_shm() {
    printf("\nTesting basic shared memory operations...\n");
    
    // Create shared memory segment
    int shmid = shmget(TEST_KEY, TEST_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        return;
    }
    printf("Created shared memory segment with ID: %d\n", shmid);
    
    // Attach to shared memory
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("shmat failed");
        return;
    }
    printf("Attached to shared memory at address: %p\n", addr);
    
    // Write test pattern
    const char* test_data = "Hello from Raspberry Pi 3!";
    strcpy(addr, test_data);
    printf("Wrote test data: %s\n", test_data);
    
    // Read back the data
    printf("Read back data: %s\n", (char*)addr);
    
    // Detach from shared memory
    if (shmdt(addr) < 0) {
        perror("shmdt failed");
        return;
    }
    printf("Detached from shared memory\n");
    
    // Clean up
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl failed");
        return;
    }
    printf("Removed shared memory segment\n");
}

void test_multiple_processes() {
    printf("\nTesting shared memory between processes...\n");
    
    // Create shared memory
    int shmid = shmget(TEST_KEY, TEST_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        return;
    }
    
    // Fork a child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    if (pid == 0) {  // Child process
        // Wait for parent to write
        sleep(1);
        
        // Attach to shared memory
        void* addr = shmat(shmid, NULL, 0);
        if (addr == (void*)-1) {
            perror("child shmat failed");
            exit(1);
        }
        
        // Read and print data
        printf("Child process read: %s\n", (char*)addr);
        
        // Write response
        strcat(addr, " Child says hi!");
        
        // Detach
        shmdt(addr);
        exit(0);
        
    } else {  // Parent process
        // Attach to shared memory
        void* addr = shmat(shmid, NULL, 0);
        if (addr == (void*)-1) {
            perror("parent shmat failed");
            return;
        }
        
        // Write data
        strcpy(addr, "Parent says hello!");
        printf("Parent wrote message\n");
        
        // Wait for child
        sleep(2);
        
        // Read final content
        printf("Final shared memory content: %s\n", (char*)addr);
        
        // Cleanup
        shmdt(addr);
        shmctl(shmid, IPC_RMID, NULL);
    }
}

int main() {
    printf("Starting IPC tests on Raspberry Pi 3...\n");
    printf("System info:\n");
    system("uname -a");
    printf("\n");
    
    test_basic_shm();
    test_multiple_processes();
    
    printf("\nAll tests completed!\n");
    return 0;
}
