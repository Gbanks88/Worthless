#include "../include/poll.h"
#include "../include/memory.h"
#include <string.h>

#define MAX_EVENTS 1024

// Event loop context
static struct {
    event_context_t* events[MAX_EVENTS];
    int running;
    int stop_requested;
} event_loop;

// Initialize event loop
int event_loop_init(void) {
    memset(&event_loop, 0, sizeof(event_loop));
    return 0;
}

// Add event to loop
int event_loop_add(int fd, short events, void (*callback)(struct pollfd*, void*), void* arg) {
    if (fd < 0 || !callback) return -1;
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_EVENTS; i++) {
        if (!event_loop.events[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create event context
    event_context_t* ctx = kmalloc(sizeof(event_context_t));
    if (!ctx) return -1;
    
    ctx->fds = kmalloc(sizeof(struct pollfd));
    if (!ctx->fds) {
        kfree(ctx);
        return -1;
    }
    
    ctx->fds->fd = fd;
    ctx->fds->events = events;
    ctx->fds->revents = 0;
    ctx->nfds = 1;
    ctx->timeout = -1;
    ctx->callback = callback;
    ctx->arg = arg;
    
    event_loop.events[slot] = ctx;
    return 0;
}

// Remove event from loop
int event_loop_remove(int fd) {
    if (fd < 0) return -1;
    
    for (int i = 0; i < MAX_EVENTS; i++) {
        if (event_loop.events[i] && event_loop.events[i]->fds->fd == fd) {
            kfree(event_loop.events[i]->fds);
            kfree(event_loop.events[i]);
            event_loop.events[i] = NULL;
            return 0;
        }
    }
    
    return -1;
}

// Run event loop
int event_loop_run(void) {
    event_loop.running = 1;
    event_loop.stop_requested = 0;
    
    struct pollfd poll_fds[MAX_EVENTS];
    unsigned int nfds = 0;
    
    while (!event_loop.stop_requested) {
        // Prepare poll array
        nfds = 0;
        for (int i = 0; i < MAX_EVENTS; i++) {
            if (event_loop.events[i]) {
                memcpy(&poll_fds[nfds], event_loop.events[i]->fds, sizeof(struct pollfd));
                nfds++;
            }
        }
        
        // Wait for events
        int ret = poll(poll_fds, nfds, -1);
        if (ret < 0) break;
        
        // Process events
        for (int i = 0; i < nfds; i++) {
            if (poll_fds[i].revents) {
                // Find corresponding event context
                for (int j = 0; j < MAX_EVENTS; j++) {
                    if (event_loop.events[j] && event_loop.events[j]->fds->fd == poll_fds[i].fd) {
                        event_loop.events[j]->fds->revents = poll_fds[i].revents;
                        event_loop.events[j]->callback(event_loop.events[j]->fds, event_loop.events[j]->arg);
                        break;
                    }
                }
            }
        }
    }
    
    event_loop.running = 0;
    return 0;
}

// Stop event loop
void event_loop_stop(void) {
    event_loop.stop_requested = 1;
}

// Poll implementation
int poll(struct pollfd* fds, unsigned int nfds, int timeout) {
    if (!fds || nfds == 0) return -1;
    
    int ready = 0;
    struct timespec start_time, current_time;
    
    // Get start time if timeout is specified
    if (timeout >= 0) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }
    
    while (1) {
        // Check each file descriptor
        for (unsigned int i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            
            if (fds[i].fd < 0) continue;
            
            // Check for readability
            if (fds[i].events & (POLLIN | POLLRDNORM)) {
                if (is_readable(fds[i].fd)) {
                    fds[i].revents |= POLLIN | POLLRDNORM;
                    ready++;
                }
            }
            
            // Check for writability
            if (fds[i].events & (POLLOUT | POLLWRNORM)) {
                if (is_writable(fds[i].fd)) {
                    fds[i].revents |= POLLOUT | POLLWRNORM;
                    ready++;
                }
            }
            
            // Check for errors
            if (has_error(fds[i].fd)) {
                fds[i].revents |= POLLERR;
                ready++;
            }
            
            // Check for hangup
            if (is_hung_up(fds[i].fd)) {
                fds[i].revents |= POLLHUP;
                ready++;
            }
        }
        
        // Return if we found active file descriptors
        if (ready > 0) break;
        
        // Check timeout
        if (timeout >= 0) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            long elapsed = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;
            
            if (elapsed >= timeout) break;
        }
        
        // Yield to other processes
        schedule();
    }
    
    return ready;
}

// Select implementation
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timespec* timeout) {
    struct pollfd poll_fds[FD_SETSIZE];
    int poll_nfds = 0;
    
    // Convert fd_sets to pollfd array
    for (int fd = 0; fd < nfds; fd++) {
        short events = 0;
        
        if (readfds && FD_ISSET(fd, readfds)) events |= POLLIN;
        if (writefds && FD_ISSET(fd, writefds)) events |= POLLOUT;
        if (exceptfds && FD_ISSET(fd, exceptfds)) events |= POLLPRI;
        
        if (events) {
            poll_fds[poll_nfds].fd = fd;
            poll_fds[poll_nfds].events = events;
            poll_fds[poll_nfds].revents = 0;
            poll_nfds++;
        }
    }
    
    // Convert timeout
    int poll_timeout = -1;
    if (timeout) {
        poll_timeout = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000;
    }
    
    // Call poll
    int ready = poll(poll_fds, poll_nfds, poll_timeout);
    if (ready <= 0) return ready;
    
    // Clear fd_sets
    if (readfds) FD_ZERO(readfds);
    if (writefds) FD_ZERO(writefds);
    if (exceptfds) FD_ZERO(exceptfds);
    
    // Convert results back to fd_sets
    for (int i = 0; i < poll_nfds; i++) {
        if (poll_fds[i].revents & (POLLIN | POLLHUP | POLLERR))
            if (readfds) FD_SET(poll_fds[i].fd, readfds);
            
        if (poll_fds[i].revents & POLLOUT)
            if (writefds) FD_SET(poll_fds[i].fd, writefds);
            
        if (poll_fds[i].revents & (POLLPRI | POLLERR))
            if (exceptfds) FD_SET(poll_fds[i].fd, exceptfds);
    }
    
    return ready;
}
