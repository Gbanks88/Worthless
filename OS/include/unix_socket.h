#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

#include <stdint.h>
#include <stddef.h>

// Unix domain socket types
#define SOCK_STREAM 1
#define SOCK_DGRAM  2

// Socket options
#define SO_RCVTIMEO 1
#define SO_SNDTIMEO 2
#define SO_RCVBUF   3
#define SO_SNDBUF   4

// Socket states
typedef enum {
    SOCKET_CREATED,
    SOCKET_BOUND,
    SOCKET_LISTENING,
    SOCKET_CONNECTED,
    SOCKET_CLOSED
} socket_state_t;

// Unix domain socket address
struct sockaddr_un {
    uint16_t sun_family;
    char sun_path[108];
};

// Unix domain socket structure
typedef struct unix_socket {
    uint32_t sock_id;
    int type;
    socket_state_t state;
    struct sockaddr_un local_addr;
    struct sockaddr_un peer_addr;
    void* recv_buffer;
    void* send_buffer;
    size_t recv_buf_size;
    size_t send_buf_size;
    struct semaphore* lock;
    struct semaphore* accept_sem;
    struct unix_socket* next;
    struct unix_socket* pending_connections;
} unix_socket_t;

// Socket operations
int unix_socket_create(int type);
int unix_socket_bind(int sockfd, const struct sockaddr_un* addr);
int unix_socket_listen(int sockfd, int backlog);
int unix_socket_accept(int sockfd, struct sockaddr_un* addr);
int unix_socket_connect(int sockfd, const struct sockaddr_un* addr);
ssize_t unix_socket_send(int sockfd, const void* buf, size_t len, int flags);
ssize_t unix_socket_recv(int sockfd, void* buf, size_t len, int flags);
int unix_socket_close(int sockfd);
int unix_socket_shutdown(int sockfd, int how);

// Socket options
int unix_socket_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
int unix_socket_getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);

// Socket status
int unix_socket_getsockname(int sockfd, struct sockaddr_un* addr, socklen_t* addrlen);
int unix_socket_getpeername(int sockfd, struct sockaddr_un* addr, socklen_t* addrlen);

#endif /* UNIX_SOCKET_H */
