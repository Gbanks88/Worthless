#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

// Network protocol types
typedef enum {
    PROTOCOL_IP,
    PROTOCOL_TCP,
    PROTOCOL_UDP,
    PROTOCOL_ICMP
} protocol_type_t;

// IP address structure
typedef struct {
    uint8_t bytes[4];
} ip_addr_t;

// Network interface structure
typedef struct {
    char name[32];
    ip_addr_t ip_addr;
    ip_addr_t netmask;
    ip_addr_t gateway;
    uint8_t mac_addr[6];
    uint32_t mtu;
    uint32_t flags;
} net_interface_t;

// Socket structure
typedef struct {
    uint32_t id;
    protocol_type_t protocol;
    uint16_t local_port;
    uint16_t remote_port;
    ip_addr_t local_addr;
    ip_addr_t remote_addr;
    uint32_t state;
    void* recv_buffer;
    void* send_buffer;
} socket_t;

// Network stack operations
int net_init(void);
int net_interface_up(const char* name);
int net_interface_down(const char* name);
int net_configure_interface(const char* name, ip_addr_t ip, ip_addr_t netmask, ip_addr_t gateway);

// Socket operations
socket_t* socket_create(protocol_type_t protocol);
int socket_bind(socket_t* socket, ip_addr_t addr, uint16_t port);
int socket_connect(socket_t* socket, ip_addr_t addr, uint16_t port);
int socket_listen(socket_t* socket, int backlog);
socket_t* socket_accept(socket_t* socket);
int socket_send(socket_t* socket, const void* data, size_t size);
int socket_recv(socket_t* socket, void* buffer, size_t size);
int socket_close(socket_t* socket);

// Protocol-specific operations
int tcp_init(void);
int udp_init(void);
int ip_init(void);
int icmp_init(void);

// Network utilities
ip_addr_t ip_addr_from_string(const char* str);
char* ip_addr_to_string(ip_addr_t addr);
uint16_t checksum(const void* data, size_t size);

#endif /* NETWORK_H */
