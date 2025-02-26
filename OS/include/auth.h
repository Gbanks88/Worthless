#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>

// User and group IDs
typedef uint32_t uid_t;
typedef uint32_t gid_t;

// User information structure
typedef struct {
    uid_t uid;
    gid_t gid;
    char username[32];
    char password_hash[64];
    char home_dir[256];
    char shell[128];
} user_info_t;

// Group information structure
typedef struct {
    gid_t gid;
    char groupname[32];
    uid_t members[32];
    int num_members;
} group_info_t;

// Authentication result
typedef enum {
    AUTH_SUCCESS,
    AUTH_FAILED,
    AUTH_ERROR
} auth_result_t;

// User management functions
int create_user(const char* username, const char* password);
int delete_user(const char* username);
int modify_user(const char* username, const user_info_t* info);
user_info_t* get_user_info(const char* username);
user_info_t* get_user_by_id(uid_t uid);

// Group management functions
int create_group(const char* groupname);
int delete_group(const char* groupname);
int add_user_to_group(const char* username, const char* groupname);
int remove_user_from_group(const char* username, const char* groupname);
group_info_t* get_group_info(const char* groupname);
group_info_t* get_group_by_id(gid_t gid);

// Authentication functions
auth_result_t authenticate_user(const char* username, const char* password);
int change_password(const char* username, const char* old_password, const char* new_password);
int check_permission(uid_t uid, gid_t gid, uint32_t required_permissions);

// Session management
typedef struct {
    uid_t uid;
    gid_t gid;
    char username[32];
    uint32_t session_id;
    uint64_t login_time;
    uint64_t last_access;
} user_session_t;

user_session_t* create_session(const char* username);
int destroy_session(user_session_t* session);
user_session_t* get_session(uint32_t session_id);
int validate_session(user_session_t* session);

// Password hashing
char* hash_password(const char* password);
int verify_password(const char* password, const char* hash);

#endif /* AUTH_H */
