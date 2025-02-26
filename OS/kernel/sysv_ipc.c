#include "../include/sysv_ipc.h"
#include "../include/memory.h"
#include <string.h>
#include <time.h>

#define MAX_SEGMENTS 128
#define PAGE_SIZE 4096

// Shared memory segment structure
typedef struct {
    struct shmid_ds ds;           // Segment descriptor
    struct shm_security security; // Security context
    void* addr;                   // Virtual address
    size_t size;                  // Size in bytes
    uint32_t* page_table;         // Page table
    int locked;                   // Memory lock flag
    void (*monitor_cb)(const char* event, void* arg);
    void* monitor_arg;
} shm_segment_t;

// Global state
static struct {
    shm_segment_t* segments[MAX_SEGMENTS];
    uint32_t num_segments;
    struct shm_info stats;
} shm_state;

// Initialize shared memory system
static void shm_init(void) {
    memset(&shm_state, 0, sizeof(shm_state));
}

// Create or get shared memory segment
int shmget(uint32_t key, size_t size, int flags) {
    // Initialize if needed
    if (shm_state.num_segments == 0) {
        shm_init();
    }
    
    // Round up size to page boundary
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Check if segment exists
    if (key != IPC_PRIVATE) {
        for (int i = 0; i < MAX_SEGMENTS; i++) {
            if (shm_state.segments[i] && 
                shm_state.segments[i]->ds.shm_perm.key == key) {
                if (flags & IPC_EXCL) {
                    return -1;
                }
                return i;
            }
        }
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        if (!shm_state.segments[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    // Create new segment
    shm_segment_t* seg = kmalloc(sizeof(shm_segment_t));
    if (!seg) return -1;
    
    // Allocate memory
    seg->addr = vmalloc(size);
    if (!seg->addr) {
        kfree(seg);
        return -1;
    }
    
    // Initialize segment
    seg->size = size;
    seg->ds.shm_perm.key = key;
    seg->ds.shm_perm.mode = flags & 0777;
    seg->ds.shm_perm.cuid = get_current_uid();
    seg->ds.shm_perm.cgid = get_current_gid();
    seg->ds.shm_segsz = size;
    seg->ds.shm_cpid = get_current_pid();
    seg->ds.shm_lpid = seg->ds.shm_cpid;
    seg->ds.shm_nattch = 0;
    seg->ds.shm_atime = 0;
    seg->ds.shm_dtime = 0;
    seg->ds.shm_ctime = time(NULL);
    seg->ds.security = SEC_NONE;
    
    // Initialize security context
    memset(&seg->security, 0, sizeof(struct shm_security));
    
    shm_state.segments[slot] = seg;
    shm_state.num_segments++;
    shm_state.stats.used_ids++;
    shm_state.stats.shm_tot += size;
    
    return slot;
}

// Attach shared memory segment
void* shmat(int shmid, const void* shmaddr, int shmflg) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return (void*)-1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    // Check permissions
    if ((shmflg & SHM_RDONLY) && !(seg->ds.shm_perm.mode & SHM_R)) {
        return (void*)-1;
    }
    
    // Decrypt if needed
    if (seg->ds.security == SEC_ENCRYPT) {
        shm_decrypt_segment(shmid);
    }
    
    // Map memory
    void* addr;
    if (shmaddr && (shmflg & SHM_RND)) {
        addr = (void*)(((uintptr_t)shmaddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    } else {
        addr = shmaddr ? (void*)shmaddr : seg->addr;
    }
    
    // Update segment info
    seg->ds.shm_nattch++;
    seg->ds.shm_atime = time(NULL);
    seg->ds.shm_lpid = get_current_pid();
    
    // Audit access
    if (seg->ds.security >= SEC_AUDIT) {
        shm_audit_access(shmid, "attach");
    }
    
    return addr;
}

// Detach shared memory segment
int shmdt(const void* shmaddr) {
    // Find segment
    shm_segment_t* seg = NULL;
    int shmid = -1;
    
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        if (shm_state.segments[i] && shm_state.segments[i]->addr == shmaddr) {
            seg = shm_state.segments[i];
            shmid = i;
            break;
        }
    }
    
    if (!seg) return -1;
    
    // Encrypt if needed
    if (seg->ds.security == SEC_ENCRYPT) {
        shm_encrypt_segment(shmid);
    }
    
    // Update segment info
    seg->ds.shm_nattch--;
    seg->ds.shm_dtime = time(NULL);
    seg->ds.shm_lpid = get_current_pid();
    
    // Audit access
    if (seg->ds.security >= SEC_AUDIT) {
        shm_audit_access(shmid, "detach");
    }
    
    return 0;
}

// Set security level
int shm_set_security(int shmid, uint8_t level, const void* key, size_t keylen) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    // Check permissions
    if (get_current_uid() != seg->ds.shm_perm.uid) {
        return -1;
    }
    
    // Set security level
    seg->ds.security = level;
    
    if (level >= SEC_ENCRYPT && key && keylen <= 32) {
        memcpy(seg->security.key, key, keylen);
        // Generate random IV
        for (int i = 0; i < 16; i++) {
            seg->security.iv[i] = rand() & 0xFF;
        }
    }
    
    return 0;
}

// Encrypt segment
int shm_encrypt_segment(int shmid) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    if (seg->ds.security != SEC_ENCRYPT) {
        return -1;
    }
    
    // Would use real encryption here
    // This is just a placeholder that XORs with the key
    uint8_t* data = (uint8_t*)seg->addr;
    for (size_t i = 0; i < seg->size; i++) {
        data[i] ^= seg->security.key[i % 32];
    }
    
    // Update MAC
    shm_verify_integrity(shmid);
    
    return 0;
}

// Decrypt segment
int shm_decrypt_segment(int shmid) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    if (seg->ds.security != SEC_ENCRYPT) {
        return -1;
    }
    
    // Verify integrity first
    if (shm_verify_integrity(shmid) != 0) {
        return -1;
    }
    
    // Would use real decryption here
    // This is just a placeholder that XORs with the key
    uint8_t* data = (uint8_t*)seg->addr;
    for (size_t i = 0; i < seg->size; i++) {
        data[i] ^= seg->security.key[i % 32];
    }
    
    return 0;
}

// Verify segment integrity
int shm_verify_integrity(int shmid) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    if (seg->ds.security < SEC_ENCRYPT) {
        return 0;
    }
    
    // Would use real MAC verification here
    // This is just a placeholder
    uint8_t mac[32];
    uint8_t* data = (uint8_t*)seg->addr;
    for (int i = 0; i < 32; i++) {
        mac[i] = 0;
        for (size_t j = 0; j < seg->size; j++) {
            mac[i] ^= data[j];
        }
        mac[i] ^= seg->security.key[i];
    }
    
    return memcmp(mac, seg->security.mac, 32) == 0 ? 0 : -1;
}

// Audit access
int shm_audit_access(int shmid, const char* action) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    
    if (seg->monitor_cb) {
        seg->monitor_cb(action, seg->monitor_arg);
    }
    
    // Would log to audit system here
    return 0;
}

// Monitor segment
int shm_monitor_segment(int shmid, void (*callback)(const char* event, void* arg), void* arg) {
    if (shmid < 0 || shmid >= MAX_SEGMENTS || !shm_state.segments[shmid]) {
        return -1;
    }
    
    shm_segment_t* seg = shm_state.segments[shmid];
    seg->monitor_cb = callback;
    seg->monitor_arg = arg;
    
    return 0;
}
