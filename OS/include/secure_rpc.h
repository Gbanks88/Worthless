#ifndef SECURE_RPC_H
#define SECURE_RPC_H

#include <stdint.h>
#include <stddef.h>

// Authentication methods
#define AUTH_NONE     0
#define AUTH_PASSWORD 1
#define AUTH_TOKEN    2
#define AUTH_CERT     3

// Encryption algorithms
#define ENC_NONE      0
#define ENC_AES_128   1
#define ENC_AES_256   2
#define ENC_ChaCha20  3

// Maximum sizes
#define MAX_KEY_SIZE    32
#define MAX_TOKEN_SIZE  256
#define MAX_CERT_SIZE   4096
#define MAX_NONCE_SIZE  24

// Authentication data
typedef struct {
    uint8_t method;
    union {
        struct {
            char username[32];
            char password[64];
        } pwd;
        struct {
            uint8_t token[MAX_TOKEN_SIZE];
            size_t token_len;
        } token;
        struct {
            uint8_t cert[MAX_CERT_SIZE];
            size_t cert_len;
        } cert;
    } data;
} auth_data_t;

// Encryption context
typedef struct {
    uint8_t algorithm;
    uint8_t key[MAX_KEY_SIZE];
    size_t key_len;
    uint8_t nonce[MAX_NONCE_SIZE];
    size_t nonce_len;
} encryption_ctx_t;

// Secure RPC session
typedef struct {
    uint32_t session_id;
    auth_data_t auth;
    encryption_ctx_t enc;
    uint64_t created_at;
    uint64_t expires_at;
} secure_session_t;

// Secure RPC message
typedef struct {
    uint32_t msg_id;
    uint32_t session_id;
    uint32_t flags;
    size_t payload_len;
    uint8_t* payload;
    uint8_t hmac[32];
} secure_rpc_msg_t;

// Server functions
int srpc_server_init(uint32_t program, uint32_t version, uint8_t auth_method, uint8_t enc_algorithm);
int srpc_server_set_auth_callback(int (*auth_cb)(auth_data_t* auth));
int srpc_server_set_key(const uint8_t* key, size_t key_len);
int srpc_server_start(const char* endpoint);
int srpc_server_stop(void);

// Client functions
secure_session_t* srpc_client_connect(const char* endpoint, auth_data_t* auth);
int srpc_client_call(secure_session_t* session, uint32_t procedure,
                    const void* request, size_t req_len,
                    void** response, size_t* resp_len);
int srpc_client_disconnect(secure_session_t* session);

// Utility functions
int srpc_encrypt_message(encryption_ctx_t* ctx, const void* input, size_t input_len,
                        void** output, size_t* output_len);
int srpc_decrypt_message(encryption_ctx_t* ctx, const void* input, size_t input_len,
                        void** output, size_t* output_len);
int srpc_generate_hmac(const void* data, size_t data_len, const uint8_t* key,
                      size_t key_len, uint8_t* hmac);
int srpc_verify_hmac(const void* data, size_t data_len, const uint8_t* key,
                     size_t key_len, const uint8_t* hmac);

// Session management
secure_session_t* srpc_create_session(auth_data_t* auth, encryption_ctx_t* enc);
int srpc_validate_session(secure_session_t* session);
void srpc_destroy_session(secure_session_t* session);

#endif /* SECURE_RPC_H */
