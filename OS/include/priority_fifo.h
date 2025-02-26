#ifndef PRIORITY_FIFO_H
#define PRIORITY_FIFO_H

#include <stdint.h>
#include <stddef.h>

// Priority levels
#define PRIO_LOW     0
#define PRIO_NORMAL  1
#define PRIO_HIGH    2
#define PRIO_URGENT  3
#define MAX_PRIORITY 3

// Priority FIFO message
typedef struct pfifo_msg {
    void* data;
    size_t size;
    uint8_t priority;
    struct pfifo_msg* next;
} pfifo_msg_t;

// Priority FIFO structure
typedef struct {
    char name[256];
    uint32_t fifo_id;
    uint32_t permissions;
    size_t total_size;
    size_t max_size;
    pfifo_msg_t* head[MAX_PRIORITY + 1];
    pfifo_msg_t* tail[MAX_PRIORITY + 1];
    uint32_t msg_count[MAX_PRIORITY + 1];
    struct semaphore* read_sem;
    struct semaphore* write_sem;
    struct semaphore* access_sem;
} priority_fifo_t;

// Priority FIFO operations
int pfifo_create(const char* name, uint32_t permissions, size_t max_size);
int pfifo_open(const char* name, int mode);
int pfifo_close(int fd);
ssize_t pfifo_read(int fd, void* buf, size_t count, uint8_t* priority);
ssize_t pfifo_write(int fd, const void* buf, size_t count, uint8_t priority);
int pfifo_get_priority_count(int fd, uint8_t priority, uint32_t* count);
int pfifo_flush(int fd, uint8_t priority);

// Priority FIFO status
struct pfifo_stat {
    size_t total_size;
    size_t max_size;
    uint32_t msg_counts[MAX_PRIORITY + 1];
    uint32_t total_msgs;
    uint32_t readers;
    uint32_t writers;
};

int pfifo_get_status(int fd, struct pfifo_stat* stat);

#endif /* PRIORITY_FIFO_H */
