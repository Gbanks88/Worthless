#include "../include/unix_socket.h"
#include "../include/memory.h"
#include "../include/ipc.h"
#include <string.h>

#define MAX_SOCKETS 1024
#define DEFAULT_BUFFER_SIZE 8192
#define MAX_BACKLOG 128

// Socket table
static unix_socket_t* sockets[MAX_SOCKETS];
static uint32_t next_socket_id = 1;

// Create Unix domain socket
int unix_socket_create(int type) {
    if (type != SOCK_STREAM && type != SOCK_DGRAM) {
        return -1;
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!sockets[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create socket structure
    unix_socket_t* sock = kmalloc(sizeof(unix_socket_t));
    if (!sock) return -1;
    
    // Initialize socket
    sock->sock_id = next_socket_id++;
    sock->type = type;
    sock->state = SOCKET_CREATED;
    memset(&sock->local_addr, 0, sizeof(struct sockaddr_un));
    memset(&sock->peer_addr, 0, sizeof(struct sockaddr_un));
    
    // Allocate buffers
    sock->recv_buffer = kmalloc(DEFAULT_BUFFER_SIZE);
    sock->send_buffer = kmalloc(DEFAULT_BUFFER_SIZE);
    if (!sock->recv_buffer || !sock->send_buffer) {
        if (sock->recv_buffer) kfree(sock->recv_buffer);
        if (sock->send_buffer) kfree(sock->send_buffer);
        kfree(sock);
        return -1;
    }
    
    sock->recv_buf_size = DEFAULT_BUFFER_SIZE;
    sock->send_buf_size = DEFAULT_BUFFER_SIZE;
    
    // Create semaphores
    sock->lock = ipc_create_semaphore(sock->sock_id * 2, 1, 1);
    sock->accept_sem = ipc_create_semaphore(sock->sock_id * 2 + 1, 0, MAX_BACKLOG);
    
    if (!sock->lock || !sock->accept_sem) {
        if (sock->lock) ipc_delete_semaphore(sock->lock->sem_id);
        if (sock->accept_sem) ipc_delete_semaphore(sock->accept_sem->sem_id);
        kfree(sock->recv_buffer);
        kfree(sock->send_buffer);
        kfree(sock);
        return -1;
    }
    
    sock->next = NULL;
    sock->pending_connections = NULL;
    
    sockets[slot] = sock;
    return sock->sock_id;
}

// Bind socket to address
int unix_socket_bind(int sockfd, const struct sockaddr_un* addr) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || !addr || sock->state != SOCKET_CREATED) {
        return -1;
    }
    
    // Check if address is already in use
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && strcmp(sockets[i]->local_addr.sun_path, addr->sun_path) == 0) {
            return -1;
        }
    }
    
    // Copy address
    memcpy(&sock->local_addr, addr, sizeof(struct sockaddr_un));
    sock->state = SOCKET_BOUND;
    
    return 0;
}

// Listen for connections
int unix_socket_listen(int sockfd, int backlog) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || sock->state != SOCKET_BOUND || sock->type != SOCK_STREAM) {
        return -1;
    }
    
    sock->state = SOCKET_LISTENING;
    return 0;
}

// Accept connection
int unix_socket_accept(int sockfd, struct sockaddr_un* addr) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || sock->state != SOCKET_LISTENING) {
        return -1;
    }
    
    // Wait for connection
    ipc_semaphore_wait(sock->accept_sem->sem_id);
    
    // Get pending connection
    ipc_semaphore_wait(sock->lock->sem_id);
    unix_socket_t* client = sock->pending_connections;
    if (client) {
        sock->pending_connections = client->next;
    }
    ipc_semaphore_signal(sock->lock->sem_id);
    
    if (!client) return -1;
    
    // Copy peer address if requested
    if (addr) {
        memcpy(addr, &client->local_addr, sizeof(struct sockaddr_un));
    }
    
    return client->sock_id;
}

// Connect to server
int unix_socket_connect(int sockfd, const struct sockaddr_un* addr) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || !addr || sock->state != SOCKET_CREATED) {
        return -1;
    }
    
    // Find server socket
    unix_socket_t* server = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && strcmp(sockets[i]->local_addr.sun_path, addr->sun_path) == 0) {
            server = sockets[i];
            break;
        }
    }
    if (!server || server->state != SOCKET_LISTENING) {
        return -1;
    }
    
    // Add to pending connections
    ipc_semaphore_wait(server->lock->sem_id);
    sock->next = server->pending_connections;
    server->pending_connections = sock;
    ipc_semaphore_signal(server->lock->sem_id);
    
    // Signal server
    ipc_semaphore_signal(server->accept_sem->sem_id);
    
    sock->state = SOCKET_CONNECTED;
    memcpy(&sock->peer_addr, addr, sizeof(struct sockaddr_un));
    
    return 0;
}

// Send data
ssize_t unix_socket_send(int sockfd, const void* buf, size_t len, int flags) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || !buf || sock->state != SOCKET_CONNECTED) {
        return -1;
    }
    
    // Find peer socket
    unix_socket_t* peer = get_socket_by_path(sock->peer_addr.sun_path);
    if (!peer) return -1;
    
    size_t bytes_sent = 0;
    const char* data = (const char*)buf;
    
    while (bytes_sent < len) {
        // Wait for space in peer's receive buffer
        size_t available = peer->recv_buf_size;
        if (available == 0) {
            if (flags & MSG_DONTWAIT) break;
            // Wait for buffer space
            continue;
        }
        
        // Copy data to peer's receive buffer
        size_t chunk = MIN(len - bytes_sent, available);
        memcpy(peer->recv_buffer, data + bytes_sent, chunk);
        bytes_sent += chunk;
    }
    
    return bytes_sent;
}

// Receive data
ssize_t unix_socket_recv(int sockfd, void* buf, size_t len, int flags) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock || !buf || sock->state != SOCKET_CONNECTED) {
        return -1;
    }
    
    size_t bytes_read = 0;
    char* data = (char*)buf;
    
    while (bytes_read < len) {
        // Check if data is available
        size_t available = sock->recv_buf_size;
        if (available == 0) {
            if (flags & MSG_DONTWAIT) break;
            // Wait for data
            continue;
        }
        
        // Copy data from receive buffer
        size_t chunk = MIN(len - bytes_read, available);
        memcpy(data + bytes_read, sock->recv_buffer, chunk);
        bytes_read += chunk;
        
        // Update receive buffer
        memmove(sock->recv_buffer, sock->recv_buffer + chunk, sock->recv_buf_size - chunk);
        sock->recv_buf_size -= chunk;
    }
    
    return bytes_read;
}

// Close socket
int unix_socket_close(int sockfd) {
    unix_socket_t* sock = get_socket_by_id(sockfd);
    if (!sock) return -1;
    
    // Clean up resources
    ipc_delete_semaphore(sock->lock->sem_id);
    ipc_delete_semaphore(sock->accept_sem->sem_id);
    kfree(sock->recv_buffer);
    kfree(sock->send_buffer);
    
    // Remove from socket table
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->sock_id == (uint32_t)sockfd) {
            sockets[i] = NULL;
            break;
        }
    }
    
    kfree(sock);
    return 0;
}

// Utility functions
static unix_socket_t* get_socket_by_id(int sockfd) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->sock_id == (uint32_t)sockfd) {
            return sockets[i];
        }
    }
    return NULL;
}

static unix_socket_t* get_socket_by_path(const char* path) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && strcmp(sockets[i]->local_addr.sun_path, path) == 0) {
            return sockets[i];
        }
    }
    return NULL;
}
