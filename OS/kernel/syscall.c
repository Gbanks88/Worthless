#include "../include/syscall.h"
#include "../include/memory.h"
#include "../include/filesystem.h"
#include "../include/network.h"
#include "../kernel/scheduler.h"

#define MAX_SYSCALLS 256

// System call table
static syscall_handler_t syscall_table[MAX_SYSCALLS];

// Initialize system calls
void init_syscalls(void) {
    // Register system call handlers
    register_syscall(SYS_EXIT, (syscall_handler_t)sys_exit);
    register_syscall(SYS_FORK, (syscall_handler_t)sys_fork);
    register_syscall(SYS_READ, (syscall_handler_t)sys_read);
    register_syscall(SYS_WRITE, (syscall_handler_t)sys_write);
    register_syscall(SYS_OPEN, (syscall_handler_t)sys_open);
    register_syscall(SYS_CLOSE, (syscall_handler_t)sys_close);
    register_syscall(SYS_WAITPID, (syscall_handler_t)sys_waitpid);
    register_syscall(SYS_EXEC, (syscall_handler_t)sys_exec);
    register_syscall(SYS_SOCKET, (syscall_handler_t)sys_socket);
    register_syscall(SYS_CONNECT, (syscall_handler_t)sys_connect);
    register_syscall(SYS_ACCEPT, (syscall_handler_t)sys_accept);
    register_syscall(SYS_SEND, (syscall_handler_t)sys_send);
    register_syscall(SYS_RECV, (syscall_handler_t)sys_recv);
    register_syscall(SYS_MKDIR, (syscall_handler_t)sys_mkdir);
    register_syscall(SYS_RMDIR, (syscall_handler_t)sys_rmdir);
    register_syscall(SYS_GETPID, (syscall_handler_t)sys_getpid);
    register_syscall(SYS_KILL, (syscall_handler_t)sys_kill);
    register_syscall(SYS_STAT, (syscall_handler_t)sys_stat);
    register_syscall(SYS_CHMOD, (syscall_handler_t)sys_chmod);
    register_syscall(SYS_CHOWN, (syscall_handler_t)sys_chown);
}

// Register a system call handler
int register_syscall(uint32_t number, syscall_handler_t handler) {
    if (number >= MAX_SYSCALLS || !handler) {
        return -1;
    }
    
    syscall_table[number] = handler;
    return 0;
}

// System call handler implementations
int64_t sys_exit(int status) {
    process_t* current = get_current_process();
    if (current) {
        terminate_process(current);
    }
    return 0;
}

int64_t sys_fork(void) {
    process_t* current = get_current_process();
    if (!current) return -1;
    
    process_t* new_process = clone_process(current);
    if (!new_process) return -1;
    
    return new_process->pid;
}

int64_t sys_read(int fd, void* buf, size_t count) {
    if (!buf || fd < 0) return -1;
    
    file_descriptor_t* file = get_file_descriptor(fd);
    if (!file) return -1;
    
    return fs_read(file, buf, count);
}

int64_t sys_write(int fd, const void* buf, size_t count) {
    if (!buf || fd < 0) return -1;
    
    file_descriptor_t* file = get_file_descriptor(fd);
    if (!file) return -1;
    
    return fs_write(file, buf, count);
}

int64_t sys_open(const char* pathname, int flags) {
    if (!pathname) return -1;
    
    file_descriptor_t* fd = fs_open(pathname, flags);
    if (!fd) return -1;
    
    return add_file_descriptor(fd);
}

int64_t sys_close(int fd) {
    if (fd < 0) return -1;
    
    file_descriptor_t* file = get_file_descriptor(fd);
    if (!file) return -1;
    
    return fs_close(file);
}

int64_t sys_socket(int domain, int type, int protocol) {
    socket_t* socket = socket_create(protocol);
    if (!socket) return -1;
    
    return add_socket_descriptor(socket);
}

int64_t sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    socket_t* socket = get_socket_descriptor(sockfd);
    if (!socket || !addr) return -1;
    
    ip_addr_t ip_addr;
    uint16_t port;
    
    // Convert sockaddr to our internal format
    if (convert_sockaddr(addr, &ip_addr, &port) < 0) {
        return -1;
    }
    
    return socket_connect(socket, ip_addr, port);
}

int64_t sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    socket_t* socket = get_socket_descriptor(sockfd);
    if (!socket) return -1;
    
    socket_t* new_socket = socket_accept(socket);
    if (!new_socket) return -1;
    
    // Convert our internal format to sockaddr if requested
    if (addr && addrlen) {
        convert_to_sockaddr(new_socket->remote_addr, new_socket->remote_port, addr, addrlen);
    }
    
    return add_socket_descriptor(new_socket);
}

int64_t sys_send(int sockfd, const void* buf, size_t len, int flags) {
    socket_t* socket = get_socket_descriptor(sockfd);
    if (!socket || !buf) return -1;
    
    return socket_send(socket, buf, len);
}

int64_t sys_recv(int sockfd, void* buf, size_t len, int flags) {
    socket_t* socket = get_socket_descriptor(sockfd);
    if (!socket || !buf) return -1;
    
    return socket_recv(socket, buf, len);
}

// Handle system call
int64_t handle_syscall(uint32_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    if (number >= MAX_SYSCALLS || !syscall_table[number]) {
        return -1;
    }
    
    return syscall_table[number](arg1, arg2, arg3, arg4, arg5);
}
