#ifndef RPC_H
#define RPC_H

#include <stdint.h>
#include <stddef.h>

// RPC message types
typedef enum {
    RPC_CALL,
    RPC_REPLY,
    RPC_ERROR
} rpc_msg_type_t;

// RPC status codes
typedef enum {
    RPC_SUCCESS,
    RPC_PROG_UNAVAIL,
    RPC_PROG_MISMATCH,
    RPC_PROC_UNAVAIL,
    RPC_GARBAGE_ARGS,
    RPC_SYSTEM_ERR
} rpc_status_t;

// RPC program version
typedef struct {
    uint32_t low;
    uint32_t high;
} rpc_version_t;

// RPC message header
typedef struct {
    uint32_t xid;           // Transaction ID
    rpc_msg_type_t type;    // Message type
    uint32_t program;       // Program number
    uint32_t version;       // Program version
    uint32_t procedure;     // Procedure number
    rpc_status_t status;    // Status code
    size_t data_len;        // Length of data
} rpc_header_t;

// RPC message
typedef struct {
    rpc_header_t header;
    void* data;
} rpc_message_t;

// RPC server structure
typedef struct {
    uint32_t program;
    uint32_t version;
    void (*dispatch)(rpc_message_t* msg);
} rpc_server_t;

// RPC client structure
typedef struct {
    uint32_t program;
    uint32_t version;
    int sock_fd;
    uint32_t next_xid;
} rpc_client_t;

// Server functions
int rpc_server_create(uint32_t program, uint32_t version, void (*dispatch)(rpc_message_t*));
int rpc_server_register(rpc_server_t* server);
int rpc_server_unregister(uint32_t program, uint32_t version);
int rpc_server_dispatch(rpc_message_t* msg);

// Client functions
rpc_client_t* rpc_client_create(uint32_t program, uint32_t version);
int rpc_client_connect(rpc_client_t* client, const char* host);
int rpc_client_call(rpc_client_t* client, uint32_t procedure, void* arg, size_t arg_len, void** result, size_t* result_len);
int rpc_client_destroy(rpc_client_t* client);

// Utility functions
int rpc_send_message(int sock_fd, rpc_message_t* msg);
int rpc_receive_message(int sock_fd, rpc_message_t* msg);
void rpc_free_message(rpc_message_t* msg);

#endif /* RPC_H */
