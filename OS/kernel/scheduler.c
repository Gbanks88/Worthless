#include "scheduler.h"
#include "../include/memory.h"

#define MAX_PROCESSES 1024
#define DEFAULT_QUANTUM 100

// Process table
static process_t* process_table[MAX_PROCESSES];
static process_t* current_process;
static process_t* ready_queue;
static uint32_t next_pid = 1;

// Initialize the scheduler
void init_scheduler(void) {
    // Clear process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }
    
    // Create idle process
    process_t* idle = create_process(idle_process, 0);
    current_process = idle;
    ready_queue = NULL;
}

// Create a new process
process_t* create_process(void (*entry_point)(void), uint32_t priority) {
    process_t* process = kmalloc(sizeof(process_t));
    if (!process) return NULL;
    
    // Initialize process control block
    process->pid = next_pid++;
    process->state = PROCESS_READY;
    process->priority = priority;
    process->time_quantum = DEFAULT_QUANTUM;
    process->cpu_time = 0;
    process->next = NULL;
    
    // Allocate and set up stack
    process->stack = kmalloc(4096);
    if (!process->stack) {
        kfree(process);
        return NULL;
    }
    
    // Set up page directory
    process->page_directory = create_page_directory();
    if (!process->page_directory) {
        kfree(process->stack);
        kfree(process);
        return NULL;
    }
    
    // Initialize stack frame for context switch
    setup_initial_stack(process, entry_point);
    
    // Add to process table
    process_table[process->pid] = process;
    
    // Add to ready queue
    add_to_ready_queue(process);
    
    return process;
}

// Schedule next process
void schedule(void) {
    if (!current_process) return;
    
    // Save current process context
    save_context();
    
    // Select next process
    process_t* next = select_next_process();
    if (next == current_process) return;
    
    // Switch to new process
    process_t* prev = current_process;
    current_process = next;
    switch_context(prev, next);
}

// Add process to ready queue (priority-based)
static void add_to_ready_queue(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_READY;
    
    if (!ready_queue || process->priority > ready_queue->priority) {
        process->next = ready_queue;
        ready_queue = process;
        return;
    }
    
    process_t* current = ready_queue;
    while (current->next && current->next->priority >= process->priority) {
        current = current->next;
    }
    
    process->next = current->next;
    current->next = process;
}

// Select next process to run
static process_t* select_next_process(void) {
    if (!ready_queue) return current_process;
    
    process_t* next = ready_queue;
    ready_queue = ready_queue->next;
    next->state = PROCESS_RUNNING;
    return next;
}

// Yield current process
void yield(void) {
    if (!current_process) return;
    
    add_to_ready_queue(current_process);
    schedule();
}

// Block current process
void block_process(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_BLOCKED;
    if (process == current_process) {
        schedule();
    }
}

// Unblock a process
void unblock_process(process_t* process) {
    if (!process || process->state != PROCESS_BLOCKED) return;
    
    add_to_ready_queue(process);
}

// Terminate a process
void terminate_process(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_TERMINATED;
    
    // Free resources
    kfree(process->stack);
    free_page_directory(process->page_directory);
    
    // Remove from process table
    process_table[process->pid] = NULL;
    
    if (process == current_process) {
        schedule();
    }
    
    kfree(process);
}
