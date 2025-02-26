#ifndef POLL_H
#define POLL_H

#include <stdint.h>

// Poll events
#define POLLIN     0x001   // Ready for reading
#define POLLPRI    0x002   // Priority data ready
#define POLLOUT    0x004   // Ready for writing
#define POLLERR    0x008   // Error
#define POLLHUP    0x010   // Hung up
#define POLLNVAL   0x020   // Invalid request
#define POLLRDNORM 0x040   // Normal data ready
#define POLLRDBAND 0x080   // Priority band data ready
#define POLLWRNORM 0x100   // Writing now will not block
#define POLLWRBAND 0x200   // Priority data may be written

// Poll file descriptor
struct pollfd {
    int fd;           // File descriptor
    short events;     // Requested events
    short revents;    // Returned events
};

// Select file descriptor sets
typedef struct {
    uint32_t fds_bits[32];
} fd_set;

// Timespec structure
struct timespec {
    long tv_sec;      // Seconds
    long tv_nsec;     // Nanoseconds
};

// Function prototypes
int poll(struct pollfd* fds, unsigned int nfds, int timeout);
int ppoll(struct pollfd* fds, unsigned int nfds, const struct timespec* timeout_ts, const void* sigmask);
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timespec* timeout);
int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timespec* timeout_ts, const void* sigmask);

// FD_SET macros
#define FD_SETSIZE 1024

#define FD_SET(fd, set) \
    ((set)->fds_bits[(fd) / 32] |= (1U << ((fd) % 32)))

#define FD_CLR(fd, set) \
    ((set)->fds_bits[(fd) / 32] &= ~(1U << ((fd) % 32)))

#define FD_ISSET(fd, set) \
    ((set)->fds_bits[(fd) / 32] & (1U << ((fd) % 32)))

#define FD_ZERO(set) \
    memset((set), 0, sizeof(fd_set))

// Event context structure
typedef struct {
    struct pollfd* fds;
    unsigned int nfds;
    int timeout;
    void (*callback)(struct pollfd* fd, void* arg);
    void* arg;
} event_context_t;

// Event loop functions
int event_loop_init(void);
int event_loop_add(int fd, short events, void (*callback)(struct pollfd*, void*), void* arg);
int event_loop_remove(int fd);
int event_loop_run(void);
void event_loop_stop(void);

#endif /* POLL_H */
