#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

// Process states
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

// Process control block
typedef struct process {
    uint32_t pid;                // Process ID
    process_state_t state;       // Current state
    uint32_t priority;           // Process priority
    void* stack;                 // Stack pointer
    void* page_directory;        // Page directory
    uint32_t time_quantum;       // Time slice
    uint32_t cpu_time;          // CPU time used
    struct process* next;        // Next process in queue
} process_t;

// Scheduler interface
void init_scheduler(void);
process_t* create_process(void (*entry_point)(void), uint32_t priority);
void schedule(void);
void yield(void);
void block_process(process_t* process);
void unblock_process(process_t* process);
void terminate_process(process_t* process);

// Platform-specific context switching
void switch_context(process_t* old, process_t* new);

#endif /* SCHEDULER_H */
