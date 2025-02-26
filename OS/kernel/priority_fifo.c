#include "../include/priority_fifo.h"
#include "../include/memory.h"
#include "../include/ipc.h"
#include <string.h>

#define MAX_PRIORITY_FIFOS 256

// Priority FIFO table
static priority_fifo_t* priority_fifos[MAX_PRIORITY_FIFOS];
static uint32_t next_pfifo_id = 0;

// Create priority FIFO
int pfifo_create(const char* name, uint32_t permissions, size_t max_size) {
    if (!name || strlen(name) >= 256 || max_size == 0) {
        return -1;
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_PRIORITY_FIFOS; i++) {
        if (!priority_fifos[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create FIFO structure
    priority_fifo_t* fifo = kmalloc(sizeof(priority_fifo_t));
    if (!fifo) return -1;
    
    // Initialize FIFO
    strncpy(fifo->name, name, 255);
    fifo->fifo_id = next_pfifo_id++;
    fifo->permissions = permissions;
    fifo->total_size = 0;
    fifo->max_size = max_size;
    
    // Initialize priority queues
    for (int i = 0; i <= MAX_PRIORITY; i++) {
        fifo->head[i] = NULL;
        fifo->tail[i] = NULL;
        fifo->msg_count[i] = 0;
    }
    
    // Create semaphores
    fifo->read_sem = ipc_create_semaphore(fifo->fifo_id * 3, 0, max_size);
    fifo->write_sem = ipc_create_semaphore(fifo->fifo_id * 3 + 1, max_size, max_size);
    fifo->access_sem = ipc_create_semaphore(fifo->fifo_id * 3 + 2, 1, 1);
    
    if (!fifo->read_sem || !fifo->write_sem || !fifo->access_sem) {
        if (fifo->read_sem) ipc_delete_semaphore(fifo->read_sem->sem_id);
        if (fifo->write_sem) ipc_delete_semaphore(fifo->write_sem->sem_id);
        if (fifo->access_sem) ipc_delete_semaphore(fifo->access_sem->sem_id);
        kfree(fifo);
        return -1;
    }
    
    priority_fifos[slot] = fifo;
    return fifo->fifo_id;
}

// Write to priority FIFO
ssize_t pfifo_write(int fd, const void* buf, size_t count, uint8_t priority) {
    if (fd < 0 || !buf || count == 0 || priority > MAX_PRIORITY) {
        return -1;
    }
    
    // Find FIFO
    priority_fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_PRIORITY_FIFOS; i++) {
        if (priority_fifos[i] && priority_fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = priority_fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    // Check if there's space
    if (fifo->total_size + count > fifo->max_size) {
        return -1;
    }
    
    // Create message
    pfifo_msg_t* msg = kmalloc(sizeof(pfifo_msg_t));
    if (!msg) return -1;
    
    msg->data = kmalloc(count);
    if (!msg->data) {
        kfree(msg);
        return -1;
    }
    
    memcpy(msg->data, buf, count);
    msg->size = count;
    msg->priority = priority;
    msg->next = NULL;
    
    // Lock FIFO
    ipc_semaphore_wait(fifo->access_sem->sem_id);
    
    // Add message to priority queue
    if (!fifo->head[priority]) {
        fifo->head[priority] = msg;
        fifo->tail[priority] = msg;
    } else {
        fifo->tail[priority]->next = msg;
        fifo->tail[priority] = msg;
    }
    
    fifo->msg_count[priority]++;
    fifo->total_size += count;
    
    // Signal readers
    ipc_semaphore_signal(fifo->read_sem->sem_id);
    
    // Unlock FIFO
    ipc_semaphore_signal(fifo->access_sem->sem_id);
    
    return count;
}

// Read from priority FIFO
ssize_t pfifo_read(int fd, void* buf, size_t count, uint8_t* priority) {
    if (fd < 0 || !buf || count == 0) {
        return -1;
    }
    
    // Find FIFO
    priority_fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_PRIORITY_FIFOS; i++) {
        if (priority_fifos[i] && priority_fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = priority_fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    // Wait for data
    ipc_semaphore_wait(fifo->read_sem->sem_id);
    
    // Lock FIFO
    ipc_semaphore_wait(fifo->access_sem->sem_id);
    
    // Find highest priority message
    pfifo_msg_t* msg = NULL;
    uint8_t msg_priority = 0;
    
    for (int i = MAX_PRIORITY; i >= 0; i--) {
        if (fifo->head[i]) {
            msg = fifo->head[i];
            msg_priority = i;
            break;
        }
    }
    
    if (!msg) {
        ipc_semaphore_signal(fifo->access_sem->sem_id);
        return -1;
    }
    
    // Copy data
    size_t to_copy = MIN(count, msg->size);
    memcpy(buf, msg->data, to_copy);
    
    if (priority) *priority = msg_priority;
    
    // Remove message from queue
    fifo->head[msg_priority] = msg->next;
    if (!fifo->head[msg_priority]) {
        fifo->tail[msg_priority] = NULL;
    }
    
    fifo->msg_count[msg_priority]--;
    fifo->total_size -= msg->size;
    
    // Signal writers
    ipc_semaphore_signal(fifo->write_sem->sem_id);
    
    // Unlock FIFO
    ipc_semaphore_signal(fifo->access_sem->sem_id);
    
    // Free message
    kfree(msg->data);
    kfree(msg);
    
    return to_copy;
}

// Get priority message count
int pfifo_get_priority_count(int fd, uint8_t priority, uint32_t* count) {
    if (fd < 0 || priority > MAX_PRIORITY || !count) {
        return -1;
    }
    
    // Find FIFO
    priority_fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_PRIORITY_FIFOS; i++) {
        if (priority_fifos[i] && priority_fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = priority_fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    *count = fifo->msg_count[priority];
    return 0;
}

// Flush priority queue
int pfifo_flush(int fd, uint8_t priority) {
    if (fd < 0 || priority > MAX_PRIORITY) {
        return -1;
    }
    
    // Find FIFO
    priority_fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_PRIORITY_FIFOS; i++) {
        if (priority_fifos[i] && priority_fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = priority_fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    // Lock FIFO
    ipc_semaphore_wait(fifo->access_sem->sem_id);
    
    // Free all messages in priority queue
    pfifo_msg_t* msg = fifo->head[priority];
    while (msg) {
        pfifo_msg_t* next = msg->next;
        fifo->total_size -= msg->size;
        kfree(msg->data);
        kfree(msg);
        msg = next;
    }
    
    fifo->head[priority] = NULL;
    fifo->tail[priority] = NULL;
    fifo->msg_count[priority] = 0;
    
    // Signal writers
    ipc_semaphore_signal(fifo->write_sem->sem_id);
    
    // Unlock FIFO
    ipc_semaphore_signal(fifo->access_sem->sem_id);
    
    return 0;
}
