#ifndef SYSV_IPC_H
#define SYSV_IPC_H

#include <stdint.h>
#include <stddef.h>

// IPC types
#define IPC_PRIVATE 0

// Permission flags
#define IPC_CREAT  01000   /* Create if key doesn't exist */
#define IPC_EXCL   02000   /* Fail if key exists */
#define IPC_NOWAIT 04000   /* Return error on wait */

// Operation flags
#define IPC_RMID 0    /* Remove identifier */
#define IPC_SET  1    /* Set options */
#define IPC_STAT 2    /* Get options */
#define IPC_INFO 3    /* Get info */

// Access modes
#define SHM_R  0400    /* Read permission */
#define SHM_W  0200    /* Write permission */

// Shared memory specific flags
#define SHM_RDONLY    010000  /* Attach read-only */
#define SHM_RND       020000  /* Round attach address */
#define SHM_REMAP     040000  /* Take-over region on attach */
#define SHM_EXEC      0100000 /* Execution permission */

// Shared memory controls
#define SHM_LOCK      11      /* Lock segment in memory */
#define SHM_UNLOCK    12      /* Unlock segment */
#define SHM_STAT      13      /* Return shmid_ds */
#define SHM_INFO      14      /* Return info */
#define SHM_STAT_ANY  15      /* Return shmid_ds, any owner */

// Security levels
#define SEC_NONE    0  /* No additional security */
#define SEC_ENCRYPT 1  /* Encrypt data */
#define SEC_SEAL    2  /* Seal memory (prevent tampering) */
#define SEC_AUDIT   3  /* Audit all access */

// IPC permission structure
struct ipc_perm {
    uint32_t uid;     /* Owner's user ID */
    uint32_t gid;     /* Owner's group ID */
    uint32_t cuid;    /* Creator's user ID */
    uint32_t cgid;    /* Creator's group ID */
    uint16_t mode;    /* Access modes */
    uint16_t seq;     /* Sequence number */
    uint32_t key;     /* Key */
};

// Shared memory segment info
struct shmid_ds {
    struct ipc_perm shm_perm;   /* Operation permission */
    size_t          shm_segsz;  /* Segment size */
    uint64_t        shm_atime;  /* Last attach time */
    uint64_t        shm_dtime;  /* Last detach time */
    uint64_t        shm_ctime;  /* Last change time */
    uint32_t        shm_cpid;   /* Creator PID */
    uint32_t        shm_lpid;   /* Last operation PID */
    uint32_t        shm_nattch; /* Number of current attaches */
    uint8_t         security;   /* Security level */
};

// Shared memory security context
struct shm_security {
    uint8_t level;              /* Security level */
    uint8_t key[32];           /* Encryption key */
    uint8_t iv[16];            /* Initialization vector */
    uint8_t mac[32];           /* Message authentication code */
    uint64_t version;          /* Version counter */
};

// Function prototypes
int shmget(uint32_t key, size_t size, int flags);
void* shmat(int shmid, const void* shmaddr, int shmflg);
int shmdt(const void* shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds* buf);

// Enhanced security functions
int shm_set_security(int shmid, uint8_t level, const void* key, size_t keylen);
int shm_verify_integrity(int shmid);
int shm_encrypt_segment(int shmid);
int shm_decrypt_segment(int shmid);
int shm_audit_access(int shmid, const char* action);

// Memory sealing functions
int shm_seal_memory(int shmid);
int shm_unseal_memory(int shmid);
int shm_verify_seal(int shmid);

// Monitoring and debugging
struct shm_info {
    uint32_t used_ids;      /* Number of currently existing segments */
    uint64_t shm_tot;       /* Total shared memory in bytes */
    uint64_t shm_rss;       /* Total resident shared memory in bytes */
    uint32_t shm_swp;       /* Total swapped shared memory in bytes */
    uint32_t swap_attempts; /* Swap attempts */
    uint32_t swap_successes;/* Successful swaps */
};

int shm_get_stats(struct shm_info* info);
int shm_monitor_segment(int shmid, void (*callback)(const char* event, void* arg), void* arg);

#endif /* SYSV_IPC_H */
