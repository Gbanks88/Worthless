#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <stddef.h>

// IPC message types
typedef enum {
    IPC_MSG_NORMAL,
    IPC_MSG_PRIORITY,
    IPC_MSG_SYSTEM
} ipc_msg_type_t;

// IPC message structure
typedef struct {
    uint32_t msg_id;
    ipc_msg_type_t type;
    uint32_t sender_pid;
    uint32_t receiver_pid;
    size_t size;
    void* data;
} ipc_message_t;

// Shared memory structure
typedef struct {
    uint32_t shm_id;
    void* address;
    size_t size;
    uint32_t owner_pid;
    uint32_t permissions;
} shared_memory_t;

// Semaphore structure
typedef struct {
    uint32_t sem_id;
    int32_t value;
    uint32_t max_value;
    uint32_t owner_pid;
} semaphore_t;

// Message queue functions
int ipc_create_queue(uint32_t queue_id);
int ipc_delete_queue(uint32_t queue_id);
int ipc_send_message(uint32_t queue_id, ipc_message_t* message);
int ipc_receive_message(uint32_t queue_id, ipc_message_t* message);

// Shared memory functions
shared_memory_t* ipc_create_shared_memory(size_t size);
int ipc_delete_shared_memory(uint32_t shm_id);
void* ipc_attach_shared_memory(uint32_t shm_id);
int ipc_detach_shared_memory(void* address);

// Semaphore functions
semaphore_t* ipc_create_semaphore(uint32_t sem_id, int32_t initial_value, uint32_t max_value);
int ipc_delete_semaphore(uint32_t sem_id);
int ipc_semaphore_wait(uint32_t sem_id);
int ipc_semaphore_signal(uint32_t sem_id);
int ipc_semaphore_try_wait(uint32_t sem_id);

// Pipe functions
int ipc_create_pipe(int pipefd[2]);
int ipc_close_pipe(int fd);
ssize_t ipc_pipe_read(int fd, void* buf, size_t count);
ssize_t ipc_pipe_write(int fd, const void* buf, size_t count);

// Signal handling
typedef void (*signal_handler_t)(int);
int ipc_signal_register(int signum, signal_handler_t handler);
int ipc_signal_send(uint32_t pid, int signum);
int ipc_signal_mask(int signum);
int ipc_signal_unmask(int signum);

#endif /* IPC_H */
