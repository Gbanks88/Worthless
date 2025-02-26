#include "../include/network.h"
#include "../include/memory.h"
#include <string.h>

#define MAX_INTERFACES 8
#define MAX_SOCKETS 1024
#define BUFFER_SIZE 8192

// Network interfaces table
static net_interface_t interfaces[MAX_INTERFACES];
static int num_interfaces = 0;

// Socket table
static socket_t* sockets[MAX_SOCKETS];
static uint32_t next_socket_id = 1;

// Initialize network stack
int net_init(void) {
    // Initialize protocol stacks
    if (ip_init() < 0) return -1;
    if (tcp_init() < 0) return -1;
    if (udp_init() < 0) return -1;
    if (icmp_init() < 0) return -1;
    
    // Clear interface and socket tables
    memset(interfaces, 0, sizeof(interfaces));
    memset(sockets, 0, sizeof(sockets));
    
    return 0;
}

// Bring network interface up
int net_interface_up(const char* name) {
    net_interface_t* iface = find_interface(name);
    if (!iface) return -1;
    
    iface->flags |= 1; // Set UP flag
    return 0;
}

// Bring network interface down
int net_interface_down(const char* name) {
    net_interface_t* iface = find_interface(name);
    if (!iface) return -1;
    
    iface->flags &= ~1; // Clear UP flag
    return 0;
}

// Configure network interface
int net_configure_interface(const char* name, ip_addr_t ip, ip_addr_t netmask, ip_addr_t gateway) {
    net_interface_t* iface = find_interface(name);
    if (!iface) {
        if (num_interfaces >= MAX_INTERFACES) return -1;
        iface = &interfaces[num_interfaces++];
        strncpy(iface->name, name, sizeof(iface->name) - 1);
    }
    
    iface->ip_addr = ip;
    iface->netmask = netmask;
    iface->gateway = gateway;
    
    return 0;
}

// Create a socket
socket_t* socket_create(protocol_type_t protocol) {
    if (next_socket_id >= MAX_SOCKETS) return NULL;
    
    socket_t* socket = kmalloc(sizeof(socket_t));
    if (!socket) return NULL;
    
    // Initialize socket
    socket->id = next_socket_id++;
    socket->protocol = protocol;
    socket->state = 0;
    socket->recv_buffer = kmalloc(BUFFER_SIZE);
    socket->send_buffer = kmalloc(BUFFER_SIZE);
    
    if (!socket->recv_buffer || !socket->send_buffer) {
        if (socket->recv_buffer) kfree(socket->recv_buffer);
        if (socket->send_buffer) kfree(socket->send_buffer);
        kfree(socket);
        return NULL;
    }
    
    sockets[socket->id] = socket;
    return socket;
}

// Bind socket to address and port
int socket_bind(socket_t* socket, ip_addr_t addr, uint16_t port) {
    if (!socket || !is_valid_socket(socket)) return -1;
    
    socket->local_addr = addr;
    socket->local_port = port;
    
    return 0;
}

// Connect socket to remote address
int socket_connect(socket_t* socket, ip_addr_t addr, uint16_t port) {
    if (!socket || !is_valid_socket(socket)) return -1;
    
    socket->remote_addr = addr;
    socket->remote_port = port;
    
    if (socket->protocol == PROTOCOL_TCP) {
        return tcp_connect(socket);
    }
    
    return 0;
}

// Listen for incoming connections
int socket_listen(socket_t* socket, int backlog) {
    if (!socket || !is_valid_socket(socket) || socket->protocol != PROTOCOL_TCP) {
        return -1;
    }
    
    return tcp_listen(socket, backlog);
}

// Accept incoming connection
socket_t* socket_accept(socket_t* socket) {
    if (!socket || !is_valid_socket(socket) || socket->protocol != PROTOCOL_TCP) {
        return NULL;
    }
    
    return tcp_accept(socket);
}

// Send data through socket
int socket_send(socket_t* socket, const void* data, size_t size) {
    if (!socket || !is_valid_socket(socket) || !data) return -1;
    
    switch (socket->protocol) {
        case PROTOCOL_TCP:
            return tcp_send(socket, data, size);
        case PROTOCOL_UDP:
            return udp_send(socket, data, size);
        default:
            return -1;
    }
}

// Receive data from socket
int socket_recv(socket_t* socket, void* buffer, size_t size) {
    if (!socket || !is_valid_socket(socket) || !buffer) return -1;
    
    switch (socket->protocol) {
        case PROTOCOL_TCP:
            return tcp_recv(socket, buffer, size);
        case PROTOCOL_UDP:
            return udp_recv(socket, buffer, size);
        default:
            return -1;
    }
}

// Close socket
int socket_close(socket_t* socket) {
    if (!socket || !is_valid_socket(socket)) return -1;
    
    // Clean up socket resources
    kfree(socket->recv_buffer);
    kfree(socket->send_buffer);
    
    sockets[socket->id] = NULL;
    kfree(socket);
    
    return 0;
}

// Utility functions
static net_interface_t* find_interface(const char* name) {
    for (int i = 0; i < num_interfaces; i++) {
        if (strcmp(interfaces[i].name, name) == 0) {
            return &interfaces[i];
        }
    }
    return NULL;
}

static int is_valid_socket(socket_t* socket) {
    return socket && socket->id < MAX_SOCKETS && sockets[socket->id] == socket;
}

ip_addr_t ip_addr_from_string(const char* str) {
    ip_addr_t addr = {0};
    if (!str) return addr;
    
    int values[4];
    if (sscanf(str, "%d.%d.%d.%d", &values[0], &values[1], &values[2], &values[3]) == 4) {
        for (int i = 0; i < 4; i++) {
            addr.bytes[i] = (uint8_t)values[i];
        }
    }
    
    return addr;
}

char* ip_addr_to_string(ip_addr_t addr) {
    static char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d",
             addr.bytes[0], addr.bytes[1], addr.bytes[2], addr.bytes[3]);
    return buffer;
}

uint16_t checksum(const void* data, size_t size) {
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (size > 1) {
        sum += *ptr++;
        size -= 2;
    }
    
    if (size > 0) {
        sum += *(const uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}
