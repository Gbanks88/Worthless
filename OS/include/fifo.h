#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stddef.h>

// FIFO modes
#define FIFO_READ  0x01
#define FIFO_WRITE 0x02
#define FIFO_NONBLOCK 0x04

// FIFO structure
typedef struct {
    char name[256];
    uint32_t fifo_id;
    uint32_t permissions;
    size_t buffer_size;
    void* buffer;
    size_t read_pos;
    size_t write_pos;
    size_t data_size;
    uint32_t readers;
    uint32_t writers;
    struct semaphore* read_sem;
    struct semaphore* write_sem;
    struct semaphore* access_sem;
} fifo_t;

// FIFO operations
int fifo_create(const char* name, uint32_t permissions);
int fifo_open(const char* name, int mode);
int fifo_close(int fd);
ssize_t fifo_read(int fd, void* buf, size_t count);
ssize_t fifo_write(int fd, const void* buf, size_t count);
int fifo_unlink(const char* name);
int fifo_get_status(int fd, struct fifo_stat* stat);

// FIFO status structure
struct fifo_stat {
    size_t size;        // Current amount of data in FIFO
    size_t capacity;    // Total FIFO capacity
    uint32_t readers;   // Number of current readers
    uint32_t writers;   // Number of current writers
    uint32_t blocked_readers; // Number of blocked readers
    uint32_t blocked_writers; // Number of blocked writers
};

#endif /* FIFO_H */
