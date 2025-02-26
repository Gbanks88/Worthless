#include "../include/fifo.h"
#include "../include/memory.h"
#include "../include/ipc.h"
#include <string.h>

#define MAX_FIFOS 256
#define DEFAULT_FIFO_SIZE 4096

// FIFO table
static fifo_t* fifos[MAX_FIFOS];
static uint32_t next_fifo_id = 0;

// Create a new named pipe
int fifo_create(const char* name, uint32_t permissions) {
    if (!name || strlen(name) >= 256) {
        return -1;
    }
    
    // Check if FIFO already exists
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (fifos[i] && strcmp(fifos[i]->name, name) == 0) {
            return -1;
        }
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (!fifos[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create FIFO structure
    fifo_t* fifo = kmalloc(sizeof(fifo_t));
    if (!fifo) return -1;
    
    // Initialize FIFO
    strncpy(fifo->name, name, 255);
    fifo->fifo_id = next_fifo_id++;
    fifo->permissions = permissions;
    fifo->buffer_size = DEFAULT_FIFO_SIZE;
    fifo->buffer = kmalloc(DEFAULT_FIFO_SIZE);
    if (!fifo->buffer) {
        kfree(fifo);
        return -1;
    }
    
    fifo->read_pos = 0;
    fifo->write_pos = 0;
    fifo->data_size = 0;
    fifo->readers = 0;
    fifo->writers = 0;
    
    // Create semaphores
    fifo->read_sem = ipc_create_semaphore(fifo->fifo_id * 3, 0, DEFAULT_FIFO_SIZE);
    fifo->write_sem = ipc_create_semaphore(fifo->fifo_id * 3 + 1, DEFAULT_FIFO_SIZE, DEFAULT_FIFO_SIZE);
    fifo->access_sem = ipc_create_semaphore(fifo->fifo_id * 3 + 2, 1, 1);
    
    if (!fifo->read_sem || !fifo->write_sem || !fifo->access_sem) {
        if (fifo->read_sem) ipc_delete_semaphore(fifo->read_sem->sem_id);
        if (fifo->write_sem) ipc_delete_semaphore(fifo->write_sem->sem_id);
        if (fifo->access_sem) ipc_delete_semaphore(fifo->access_sem->sem_id);
        kfree(fifo->buffer);
        kfree(fifo);
        return -1;
    }
    
    fifos[slot] = fifo;
    return fifo->fifo_id;
}

// Open an existing FIFO
int fifo_open(const char* name, int mode) {
    if (!name) return -1;
    
    fifo_t* fifo = NULL;
    
    // Find FIFO by name
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (fifos[i] && strcmp(fifos[i]->name, name) == 0) {
            fifo = fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    // Check permissions
    if ((mode & FIFO_READ) && !(fifo->permissions & FIFO_READ)) return -1;
    if ((mode & FIFO_WRITE) && !(fifo->permissions & FIFO_WRITE)) return -1;
    
    // Update reader/writer count
    ipc_semaphore_wait(fifo->access_sem->sem_id);
    if (mode & FIFO_READ) fifo->readers++;
    if (mode & FIFO_WRITE) fifo->writers++;
    ipc_semaphore_signal(fifo->access_sem->sem_id);
    
    return fifo->fifo_id;
}

// Read from FIFO
ssize_t fifo_read(int fd, void* buf, size_t count) {
    if (fd < 0 || !buf || count == 0) return -1;
    
    fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (fifos[i] && fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    size_t bytes_read = 0;
    
    while (bytes_read < count) {
        // Wait for data
        if (ipc_semaphore_wait(fifo->read_sem->sem_id) < 0) {
            return bytes_read > 0 ? bytes_read : -1;
        }
        
        // Lock FIFO
        ipc_semaphore_wait(fifo->access_sem->sem_id);
        
        // Read data
        size_t available = MIN(count - bytes_read, fifo->data_size);
        size_t chunk = MIN(available, fifo->buffer_size - fifo->read_pos);
        
        memcpy(buf + bytes_read, fifo->buffer + fifo->read_pos, chunk);
        fifo->read_pos = (fifo->read_pos + chunk) % fifo->buffer_size;
        fifo->data_size -= chunk;
        bytes_read += chunk;
        
        // Signal write space available
        ipc_semaphore_signal(fifo->write_sem->sem_id);
        
        // Unlock FIFO
        ipc_semaphore_signal(fifo->access_sem->sem_id);
        
        if (bytes_read == count) break;
    }
    
    return bytes_read;
}

// Write to FIFO
ssize_t fifo_write(int fd, const void* buf, size_t count) {
    if (fd < 0 || !buf || count == 0) return -1;
    
    fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (fifos[i] && fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    size_t bytes_written = 0;
    
    while (bytes_written < count) {
        // Wait for space
        if (ipc_semaphore_wait(fifo->write_sem->sem_id) < 0) {
            return bytes_written > 0 ? bytes_written : -1;
        }
        
        // Lock FIFO
        ipc_semaphore_wait(fifo->access_sem->sem_id);
        
        // Write data
        size_t available = MIN(count - bytes_written, fifo->buffer_size - fifo->data_size);
        size_t chunk = MIN(available, fifo->buffer_size - fifo->write_pos);
        
        memcpy(fifo->buffer + fifo->write_pos, buf + bytes_written, chunk);
        fifo->write_pos = (fifo->write_pos + chunk) % fifo->buffer_size;
        fifo->data_size += chunk;
        bytes_written += chunk;
        
        // Signal data available
        ipc_semaphore_signal(fifo->read_sem->sem_id);
        
        // Unlock FIFO
        ipc_semaphore_signal(fifo->access_sem->sem_id);
        
        if (bytes_written == count) break;
    }
    
    return bytes_written;
}

// Close FIFO
int fifo_close(int fd) {
    if (fd < 0) return -1;
    
    fifo_t* fifo = NULL;
    for (int i = 0; i < MAX_FIFOS; i++) {
        if (fifos[i] && fifos[i]->fifo_id == (uint32_t)fd) {
            fifo = fifos[i];
            break;
        }
    }
    if (!fifo) return -1;
    
    // Update reader/writer count
    ipc_semaphore_wait(fifo->access_sem->sem_id);
    if (fifo->readers > 0) fifo->readers--;
    if (fifo->writers > 0) fifo->writers--;
    
    // If no more readers and writers, clean up
    if (fifo->readers == 0 && fifo->writers == 0) {
        ipc_semaphore_signal(fifo->access_sem->sem_id);
        
        // Delete semaphores
        ipc_delete_semaphore(fifo->read_sem->sem_id);
        ipc_delete_semaphore(fifo->write_sem->sem_id);
        ipc_delete_semaphore(fifo->access_sem->sem_id);
        
        // Free memory
        kfree(fifo->buffer);
        kfree(fifo);
        
        // Remove from table
        for (int i = 0; i < MAX_FIFOS; i++) {
            if (fifos[i] && fifos[i]->fifo_id == (uint32_t)fd) {
                fifos[i] = NULL;
                break;
            }
        }
    } else {
        ipc_semaphore_signal(fifo->access_sem->sem_id);
    }
    
    return 0;
}
