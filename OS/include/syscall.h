#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "filesystem.h"
#include "network.h"

// System call numbers
enum syscall_numbers {
    SYS_EXIT = 1,
    SYS_FORK = 2,
    SYS_READ = 3,
    SYS_WRITE = 4,
    SYS_OPEN = 5,
    SYS_CLOSE = 6,
    SYS_WAITPID = 7,
    SYS_EXEC = 8,
    SYS_SOCKET = 9,
    SYS_CONNECT = 10,
    SYS_ACCEPT = 11,
    SYS_SEND = 12,
    SYS_RECV = 13,
    SYS_MKDIR = 14,
    SYS_RMDIR = 15,
    SYS_GETPID = 16,
    SYS_KILL = 17,
    SYS_STAT = 18,
    SYS_CHMOD = 19,
    SYS_CHOWN = 20
};

// System call handler type
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

// Initialize system call interface
void init_syscalls(void);

// Register system call handler
int register_syscall(uint32_t number, syscall_handler_t handler);

// System call handlers
int64_t sys_exit(int status);
int64_t sys_fork(void);
int64_t sys_read(int fd, void* buf, size_t count);
int64_t sys_write(int fd, const void* buf, size_t count);
int64_t sys_open(const char* pathname, int flags);
int64_t sys_close(int fd);
int64_t sys_waitpid(int pid, int* status, int options);
int64_t sys_exec(const char* pathname, char* const argv[], char* const envp[]);
int64_t sys_socket(int domain, int type, int protocol);
int64_t sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int64_t sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int64_t sys_send(int sockfd, const void* buf, size_t len, int flags);
int64_t sys_recv(int sockfd, void* buf, size_t len, int flags);
int64_t sys_mkdir(const char* pathname, mode_t mode);
int64_t sys_rmdir(const char* pathname);
int64_t sys_getpid(void);
int64_t sys_kill(int pid, int sig);
int64_t sys_stat(const char* pathname, struct stat* statbuf);
int64_t sys_chmod(const char* pathname, mode_t mode);
int64_t sys_chown(const char* pathname, uid_t owner, gid_t group);

#endif /* SYSCALL_H */
