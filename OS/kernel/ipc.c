#include "../include/ipc.h"
#include "../include/memory.h"
#include "../kernel/scheduler.h"
#include <string.h>

#define MAX_QUEUES 256
#define MAX_MESSAGES 1024
#define MAX_SHARED_MEMORY 128
#define MAX_SEMAPHORES 256
#define MAX_PIPES 1024

// Message queue structure
typedef struct {
    uint32_t queue_id;
    ipc_message_t* messages[MAX_MESSAGES];
    int head;
    int tail;
    int count;
    semaphore_t* lock;
    semaphore_t* not_empty;
    semaphore_t* not_full;
} message_queue_t;

// Global IPC structures
static message_queue_t* message_queues[MAX_QUEUES];
static shared_memory_t* shared_memories[MAX_SHARED_MEMORY];
static semaphore_t* semaphores[MAX_SEMAPHORES];
static int pipes[MAX_PIPES][2];

// Initialize message queue
int ipc_create_queue(uint32_t queue_id) {
    if (queue_id >= MAX_QUEUES || message_queues[queue_id]) {
        return -1;
    }
    
    message_queue_t* queue = kmalloc(sizeof(message_queue_t));
    if (!queue) return -1;
    
    queue->queue_id = queue_id;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    // Create synchronization primitives
    queue->lock = ipc_create_semaphore(queue_id * 3, 1, 1);
    queue->not_empty = ipc_create_semaphore(queue_id * 3 + 1, 0, MAX_MESSAGES);
    queue->not_full = ipc_create_semaphore(queue_id * 3 + 2, MAX_MESSAGES, MAX_MESSAGES);
    
    if (!queue->lock || !queue->not_empty || !queue->not_full) {
        if (queue->lock) ipc_delete_semaphore(queue->lock->sem_id);
        if (queue->not_empty) ipc_delete_semaphore(queue->not_empty->sem_id);
        if (queue->not_full) ipc_delete_semaphore(queue->not_full->sem_id);
        kfree(queue);
        return -1;
    }
    
    message_queues[queue_id] = queue;
    return 0;
}

// Send message to queue
int ipc_send_message(uint32_t queue_id, ipc_message_t* message) {
    if (queue_id >= MAX_QUEUES || !message_queues[queue_id] || !message) {
        return -1;
    }
    
    message_queue_t* queue = message_queues[queue_id];
    
    // Wait for space in queue
    if (ipc_semaphore_wait(queue->not_full->sem_id) < 0) {
        return -1;
    }
    
    // Lock queue
    if (ipc_semaphore_wait(queue->lock->sem_id) < 0) {
        ipc_semaphore_signal(queue->not_full->sem_id);
        return -1;
    }
    
    // Copy message
    ipc_message_t* msg_copy = kmalloc(sizeof(ipc_message_t));
    if (!msg_copy) {
        ipc_semaphore_signal(queue->lock->sem_id);
        ipc_semaphore_signal(queue->not_full->sem_id);
        return -1;
    }
    
    memcpy(msg_copy, message, sizeof(ipc_message_t));
    
    if (message->size > 0 && message->data) {
        msg_copy->data = kmalloc(message->size);
        if (!msg_copy->data) {
            kfree(msg_copy);
            ipc_semaphore_signal(queue->lock->sem_id);
            ipc_semaphore_signal(queue->not_full->sem_id);
            return -1;
        }
        memcpy(msg_copy->data, message->data, message->size);
    }
    
    // Add to queue
    queue->messages[queue->tail] = msg_copy;
    queue->tail = (queue->tail + 1) % MAX_MESSAGES;
    queue->count++;
    
    // Unlock queue
    ipc_semaphore_signal(queue->lock->sem_id);
    
    // Signal message available
    ipc_semaphore_signal(queue->not_empty->sem_id);
    
    return 0;
}

// Receive message from queue
int ipc_receive_message(uint32_t queue_id, ipc_message_t* message) {
    if (queue_id >= MAX_QUEUES || !message_queues[queue_id] || !message) {
        return -1;
    }
    
    message_queue_t* queue = message_queues[queue_id];
    
    // Wait for message
    if (ipc_semaphore_wait(queue->not_empty->sem_id) < 0) {
        return -1;
    }
    
    // Lock queue
    if (ipc_semaphore_wait(queue->lock->sem_id) < 0) {
        ipc_semaphore_signal(queue->not_empty->sem_id);
        return -1;
    }
    
    // Get message
    ipc_message_t* msg = queue->messages[queue->head];
    queue->messages[queue->head] = NULL;
    queue->head = (queue->head + 1) % MAX_MESSAGES;
    queue->count--;
    
    // Copy message to user
    memcpy(message, msg, sizeof(ipc_message_t));
    if (msg->size > 0 && msg->data) {
        message->data = kmalloc(msg->size);
        if (!message->data) {
            kfree(msg->data);
            kfree(msg);
            ipc_semaphore_signal(queue->lock->sem_id);
            ipc_semaphore_signal(queue->not_full->sem_id);
            return -1;
        }
        memcpy(message->data, msg->data, msg->size);
        kfree(msg->data);
    }
    kfree(msg);
    
    // Unlock queue
    ipc_semaphore_signal(queue->lock->sem_id);
    
    // Signal space available
    ipc_semaphore_signal(queue->not_full->sem_id);
    
    return 0;
}

// Create shared memory
shared_memory_t* ipc_create_shared_memory(size_t size) {
    if (size == 0) return NULL;
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_SHARED_MEMORY; i++) {
        if (!shared_memories[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return NULL;
    
    // Allocate shared memory
    shared_memory_t* shm = kmalloc(sizeof(shared_memory_t));
    if (!shm) return NULL;
    
    shm->address = kmalloc(size);
    if (!shm->address) {
        kfree(shm);
        return NULL;
    }
    
    shm->shm_id = slot;
    shm->size = size;
    shm->owner_pid = get_current_process()->pid;
    shm->permissions = 0666; // rw-rw-rw-
    
    shared_memories[slot] = shm;
    return shm;
}

// Create semaphore
semaphore_t* ipc_create_semaphore(uint32_t sem_id, int32_t initial_value, uint32_t max_value) {
    if (sem_id >= MAX_SEMAPHORES || semaphores[sem_id]) {
        return NULL;
    }
    
    semaphore_t* sem = kmalloc(sizeof(semaphore_t));
    if (!sem) return NULL;
    
    sem->sem_id = sem_id;
    sem->value = initial_value;
    sem->max_value = max_value;
    sem->owner_pid = get_current_process()->pid;
    
    semaphores[sem_id] = sem;
    return sem;
}

// Wait on semaphore
int ipc_semaphore_wait(uint32_t sem_id) {
    if (sem_id >= MAX_SEMAPHORES || !semaphores[sem_id]) {
        return -1;
    }
    
    semaphore_t* sem = semaphores[sem_id];
    
    while (sem->value <= 0) {
        // Block current process
        block_process(get_current_process());
        schedule();
    }
    
    sem->value--;
    return 0;
}

// Signal semaphore
int ipc_semaphore_signal(uint32_t sem_id) {
    if (sem_id >= MAX_SEMAPHORES || !semaphores[sem_id]) {
        return -1;
    }
    
    semaphore_t* sem = semaphores[sem_id];
    
    if (sem->value < sem->max_value) {
        sem->value++;
        // Wake up one waiting process
        wake_up_blocked_process();
    }
    
    return 0;
}

// Create pipe
int ipc_create_pipe(int pipefd[2]) {
    // Find free pipe slot
    int slot = -1;
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipes[i][0] == 0 && pipes[i][1] == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create pipe file descriptors
    pipes[slot][0] = slot * 2 + 1;     // Read end
    pipes[slot][1] = slot * 2 + 2;     // Write end
    
    pipefd[0] = pipes[slot][0];
    pipefd[1] = pipes[slot][1];
    
    return 0;
}
