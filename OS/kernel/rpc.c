#include "../include/rpc.h"
#include "../include/unix_socket.h"
#include "../include/memory.h"
#include <string.h>

#define MAX_RPC_SERVERS 256
#define RPC_PORT_BASE 2000

// RPC server table
static rpc_server_t* rpc_servers[MAX_RPC_SERVERS];

// Create RPC server
int rpc_server_create(uint32_t program, uint32_t version, void (*dispatch)(rpc_message_t*)) {
    if (!dispatch) return -1;
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_RPC_SERVERS; i++) {
        if (!rpc_servers[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create server structure
    rpc_server_t* server = kmalloc(sizeof(rpc_server_t));
    if (!server) return -1;
    
    server->program = program;
    server->version = version;
    server->dispatch = dispatch;
    
    rpc_servers[slot] = server;
    return 0;
}

// Register RPC server
int rpc_server_register(rpc_server_t* server) {
    if (!server) return -1;
    
    // Create socket
    int sock = unix_socket_create(SOCK_STREAM);
    if (sock < 0) return -1;
    
    // Bind to address
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/rpc_%u_%u.sock",
             server->program, server->version);
    
    if (unix_socket_bind(sock, &addr) < 0) {
        unix_socket_close(sock);
        return -1;
    }
    
    // Listen for connections
    if (unix_socket_listen(sock, 5) < 0) {
        unix_socket_close(sock);
        return -1;
    }
    
    // Accept and handle connections
    while (1) {
        int client = unix_socket_accept(sock, NULL);
        if (client < 0) continue;
        
        // Handle client in new thread
        handle_rpc_client(client, server);
    }
    
    return 0;
}

// Handle RPC client connection
static void handle_rpc_client(int sock_fd, rpc_server_t* server) {
    while (1) {
        // Receive message
        rpc_message_t msg;
        if (rpc_receive_message(sock_fd, &msg) < 0) {
            break;
        }
        
        // Verify program and version
        if (msg.header.program != server->program ||
            msg.header.version != server->version) {
            msg.header.type = RPC_ERROR;
            msg.header.status = RPC_PROG_MISMATCH;
            rpc_send_message(sock_fd, &msg);
            rpc_free_message(&msg);
            continue;
        }
        
        // Dispatch message
        server->dispatch(&msg);
        
        // Send response
        if (rpc_send_message(sock_fd, &msg) < 0) {
            rpc_free_message(&msg);
            break;
        }
        
        rpc_free_message(&msg);
    }
    
    unix_socket_close(sock_fd);
}

// Create RPC client
rpc_client_t* rpc_client_create(uint32_t program, uint32_t version) {
    rpc_client_t* client = kmalloc(sizeof(rpc_client_t));
    if (!client) return NULL;
    
    client->program = program;
    client->version = version;
    client->sock_fd = -1;
    client->next_xid = 1;
    
    return client;
}

// Connect to RPC server
int rpc_client_connect(rpc_client_t* client, const char* host) {
    if (!client || !host) return -1;
    
    // Create socket
    client->sock_fd = unix_socket_create(SOCK_STREAM);
    if (client->sock_fd < 0) return -1;
    
    // Connect to server
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/rpc_%u_%u.sock",
             client->program, client->version);
    
    if (unix_socket_connect(client->sock_fd, &addr) < 0) {
        unix_socket_close(client->sock_fd);
        client->sock_fd = -1;
        return -1;
    }
    
    return 0;
}

// Make RPC call
int rpc_client_call(rpc_client_t* client, uint32_t procedure,
                    void* arg, size_t arg_len,
                    void** result, size_t* result_len) {
    if (!client || client->sock_fd < 0) return -1;
    
    // Prepare call message
    rpc_message_t msg;
    msg.header.xid = client->next_xid++;
    msg.header.type = RPC_CALL;
    msg.header.program = client->program;
    msg.header.version = client->version;
    msg.header.procedure = procedure;
    msg.header.data_len = arg_len;
    msg.data = arg;
    
    // Send call
    if (rpc_send_message(client->sock_fd, &msg) < 0) {
        return -1;
    }
    
    // Receive reply
    rpc_message_t reply;
    if (rpc_receive_message(client->sock_fd, &reply) < 0) {
        return -1;
    }
    
    // Check reply
    if (reply.header.xid != msg.header.xid ||
        reply.header.type == RPC_ERROR) {
        rpc_free_message(&reply);
        return -1;
    }
    
    // Return result
    if (result) *result = reply.data;
    if (result_len) *result_len = reply.header.data_len;
    
    return reply.header.status;
}

// Send RPC message
int rpc_send_message(int sock_fd, rpc_message_t* msg) {
    if (sock_fd < 0 || !msg) return -1;
    
    // Send header
    if (unix_socket_send(sock_fd, &msg->header, sizeof(rpc_header_t), 0) < 0) {
        return -1;
    }
    
    // Send data if present
    if (msg->data && msg->header.data_len > 0) {
        if (unix_socket_send(sock_fd, msg->data, msg->header.data_len, 0) < 0) {
            return -1;
        }
    }
    
    return 0;
}

// Receive RPC message
int rpc_receive_message(int sock_fd, rpc_message_t* msg) {
    if (sock_fd < 0 || !msg) return -1;
    
    // Receive header
    if (unix_socket_recv(sock_fd, &msg->header, sizeof(rpc_header_t), 0) < 0) {
        return -1;
    }
    
    // Receive data if present
    if (msg->header.data_len > 0) {
        msg->data = kmalloc(msg->header.data_len);
        if (!msg->data) return -1;
        
        if (unix_socket_recv(sock_fd, msg->data, msg->header.data_len, 0) < 0) {
            kfree(msg->data);
            return -1;
        }
    } else {
        msg->data = NULL;
    }
    
    return 0;
}

// Free RPC message
void rpc_free_message(rpc_message_t* msg) {
    if (!msg) return;
    if (msg->data) kfree(msg->data);
    msg->data = NULL;
}
