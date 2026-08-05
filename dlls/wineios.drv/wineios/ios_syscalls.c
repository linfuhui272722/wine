/*
 * iOS Syscall Bridge Implementation
 *
 * Copyright 2024 Wine Project
 *
 * Provides Unix/Linux syscall mappings for iOS.
 * This is the foundation layer that Wine uses to interact with iOS.
 */

#include "ios_syscalls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <dirent.h>
#include <libkern/OSCacheControl.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <execinfo.h>

#pragma clang diagnostic ignored "-Wunused-function"

#define TRACE_SYSCALLS 0

#if TRACE_SYSCALLS
#define TRACE(fmt, ...) fprintf(stderr, "[ios-syscall] " fmt "\n", ##__VA_ARGS__)
#else
#define TRACE(fmt, ...) do {} while(0)
#endif

#define WARN(fmt, ...) fprintf(stderr, "[ios-syscall:warn] " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "[ios-syscall:err] " fmt "\n", ##__VA_ARGS__)

/* Global state */
static bool g_initialized = false;
static pthread_mutex_t g_jit_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *g_jit_region = NULL;
static size_t g_jit_used = 0;

/*
 * Memory management
 */

/* Allocate RWX memory for JIT code */
void *ios_alloc_jit_memory(size_t size)
{
    TRACE("ios_alloc_jit_memory(size=%zu)", size);
    
    if (!IOS_JAILBROKEN) {
        WARN("JIT allocation not available on non-jailbroken iOS");
        return NULL;
    }
    
    pthread_mutex_lock(&g_jit_mutex);
    
    /* Align size to page boundary */
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    /* Check if we need to create the JIT region */
    if (!g_jit_region) {
        /* Map a large RWX region for JIT code */
        g_jit_region = mmap((void *)IOS_JIT_REGION_BASE, IOS_JIT_REGION_SIZE,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                             -1, 0);
        
        if (g_jit_region == MAP_FAILED) {
            /* Try without fixed address */
            g_jit_region = mmap(NULL, IOS_JIT_REGION_SIZE,
                                PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS,
                                -1, 0);
            
            if (g_jit_region == MAP_FAILED) {
                ERR("Failed to allocate JIT region: %s", strerror(errno));
                pthread_mutex_unlock(&g_jit_mutex);
                return NULL;
            }
        }
        
        TRACE("JIT region allocated at %p, size %dMB", 
              g_jit_region, IOS_JIT_REGION_SIZE / (1024 * 1024));
        g_jit_used = 0;
    }
    
    /* Check if we have enough space */
    if (g_jit_used + size > IOS_JIT_REGION_SIZE) {
        ERR("JIT region exhausted");
        pthread_mutex_unlock(&g_jit_mutex);
        return NULL;
    }
    
    void *result = (char *)g_jit_region + g_jit_used;
    g_jit_used += size;
    
    pthread_mutex_unlock(&g_jit_mutex);
    
    TRACE("JIT memory allocated at %p, used %zu/%d", 
          result, g_jit_used, IOS_JIT_REGION_SIZE);
    
    return result;
}

/* Free JIT memory */
int ios_free_jit_memory(void *addr, size_t size)
{
    TRACE("ios_free_jit_memory(addr=%p, size=%zu)", addr, size);
    
    /* For simplicity, we don't actually free JIT memory */
    /* In a production implementation, we'd track individual allocations */
    return 0;
}

/* Map memory with specified protection */
void *ios_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    TRACE("ios_mmap(addr=%p, len=%zu, prot=%d, flags=%d, fd=%d, offset=%ld)",
          addr, len, prot, flags, fd, (long)offset);
    
    int ios_prot = 0;
    if (prot & IOS_PROT_READ) ios_prot |= PROT_READ;
    if (prot & IOS_PROT_WRITE) ios_prot |= PROT_WRITE;
    if (prot & IOS_PROT_EXEC) ios_prot |= PROT_EXEC;
    
    int ios_flags = 0;
    if (flags & IOS_MAP_PRIVATE) ios_flags |= MAP_PRIVATE;
    if (flags & IOS_MAP_SHARED) ios_flags |= MAP_SHARED;
    if (flags & IOS_MAP_ANONYMOUS) ios_flags |= MAP_ANONYMOUS;
    if (flags & IOS_MAP_FIXED) ios_flags |= MAP_FIXED;
    if (flags & IOS_MAP_JIT) ios_flags |= 0x0800;  /* JIT flag */
    
    void *result = mmap(addr, len, ios_prot, ios_flags, fd, offset);
    
    if (result == MAP_FAILED) {
        TRACE("mmap failed: %s", strerror(errno));
        return NULL;
    }
    
    return result;
}

/* Unmap memory */
int ios_munmap(void *addr, size_t len)
{
    TRACE("ios_munmap(addr=%p, len=%zu)", addr, len);
    return munmap(addr, len);
}

/* Change memory protection */
int ios_mprotect(void *addr, size_t len, int prot)
{
    TRACE("ios_mprotect(addr=%p, len=%zu, prot=%d)", addr, len, prot);
    
    int ios_prot = 0;
    if (prot & IOS_PROT_READ) ios_prot |= PROT_READ;
    if (prot & IOS_PROT_WRITE) ios_prot |= PROT_WRITE;
    if (prot & IOS_PROT_EXEC) ios_prot |= PROT_EXEC;
    
    return mprotect(addr, len, ios_prot);
}

/* Flush instruction cache */
void ios_icache_sync(void *addr, size_t len)
{
    TRACE("ios_icache_sync(addr=%p, len=%zu)", addr, len);
    sys_icache_invalidate(addr, len);
}

/*
 * Thread management
 */

/* Thread-local storage key */
static pthread_key_t g_tls_key;
static bool g_tls_key_init = false;

/* Thread start argument */
struct thread_start_arg {
    void (*entry)(void *);
    void *arg;
    pthread_mutex_t *ready_mutex;
    pthread_cond_t *ready_cond;
    bool *ready;
};

/* Thread start function */
static void *thread_start(void *arg)
{
    struct thread_start_arg *start_arg = (struct thread_start_arg *)arg;
    
    void (*entry)(void *) = start_arg->entry;
    void *thread_arg = start_arg->arg;
    
    /* Signal that we're ready */
    pthread_mutex_lock(start_arg->ready_mutex);
    *start_arg->ready = true;
    pthread_cond_signal(start_arg->ready_cond);
    pthread_mutex_unlock(start_arg->ready_mutex);
    
    /* Call the actual thread entry */
    entry(thread_arg);
    
    return NULL;
}

/* Create a new thread */
int ios_thread_create(uint64_t *thread, void *stack, size_t stack_size, 
                      void (*entry)(void *), void *arg)
{
    TRACE("ios_thread_create(stack=%p, stack_size=%zu, entry=%p, arg=%p)",
          stack, stack_size, entry, arg);
    
    pthread_t pthread;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    if (stack) {
        pthread_attr_setstackaddr(&attr, stack);
    }
    
    struct thread_start_arg start_arg;
    start_arg.entry = entry;
    start_arg.arg = arg;
    
    pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;
    bool ready = false;
    start_arg.ready_mutex = &ready_mutex;
    start_arg.ready_cond = &ready_cond;
    start_arg.ready = &ready;
    
    int result = pthread_create(&pthread, &attr, thread_start, &start_arg);
    
    if (result != 0) {
        ERR("pthread_create failed: %d", result);
        return result;
    }
    
    /* Wait for thread to start */
    pthread_mutex_lock(&ready_mutex);
    while (!ready) {
        pthread_cond_wait(&ready_cond, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
    
    pthread_attr_destroy(&attr);
    
    /* Return thread handle (use pthread_t as uint64_t) */
    *thread = (uint64_t)(uintptr_t)pthread;
    
    TRACE("Thread created: %lu", (unsigned long)pthread);
    
    return 0;
}

/* Terminate current thread */
__attribute__((noreturn)) void ios_thread_exit(void *retval)
{
    TRACE("ios_thread_exit(retval=%p)", retval);
    pthread_exit(retval);
}

/* Join a thread */
int ios_thread_join(uint64_t thread, void **retval)
{
    TRACE("ios_thread_join(thread=%lu)", (unsigned long)thread);
    return pthread_join((pthread_t)(uintptr_t)thread, retval);
}

/* Set thread name */
int ios_thread_setname(const char *name)
{
    TRACE("ios_thread_setname(name=%s)", name);
    return pthread_setname_np(name);
}

/* Get thread name */
int ios_thread_getname(char *name, size_t len)
{
    TRACE("ios_thread_getname(len=%zu)", len);
    return pthread_getname_np(pthread_self(), name, len);
}

/*
 * Synchronization primitives
 */

/* Mutex operations */
int ios_mutex_init(void **mutex)
{
    TRACE("ios_mutex_init()");
    pthread_mutex_t *m = malloc(sizeof(pthread_mutex_t));
    int result = pthread_mutex_init(m, NULL);
    if (result == 0) {
        *mutex = m;
    }
    return result;
}

int ios_mutex_lock(void *mutex)
{
    return pthread_mutex_lock((pthread_mutex_t *)mutex);
}

int ios_mutex_unlock(void *mutex)
{
    return pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

int ios_mutex_destroy(void *mutex)
{
    int result = pthread_mutex_destroy((pthread_mutex_t *)mutex);
    free(mutex);
    return result;
}

/* Semaphore operations */
int ios_sem_init(void **sem, int value)
{
    TRACE("ios_sem_init(value=%d)", value);
    sem_t *s = malloc(sizeof(sem_t));
    int result = sem_init(s, 0, value);
    if (result == 0) {
        *sem = s;
    }
    return result;
}

int ios_sem_wait(void *sem)
{
    return sem_wait((sem_t *)sem);
}

int ios_sem_post(void *sem)
{
    return sem_post((sem_t *)sem);
}

int ios_sem_destroy(void *sem)
{
    int result = sem_destroy((sem_t *)sem);
    free(sem);
    return result;
}

/* Condition variable operations */
int ios_cond_init(void **cond)
{
    TRACE("ios_cond_init()");
    pthread_cond_t *c = malloc(sizeof(pthread_cond_t));
    int result = pthread_cond_init(c, NULL);
    if (result == 0) {
        *cond = c;
    }
    return result;
}

int ios_cond_wait(void *cond, void *mutex)
{
    return pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mutex);
}

int ios_cond_signal(void *cond)
{
    return pthread_cond_signal((pthread_cond_t *)cond);
}

int ios_cond_broadcast(void *cond)
{
    return pthread_cond_broadcast((pthread_cond_t *)cond);
}

int ios_cond_destroy(void *cond)
{
    int result = pthread_cond_destroy((pthread_cond_t *)cond);
    free(cond);
    return result;
}

/*
 * File operations
 */

/* Open file */
int ios_open(const char *path, int flags, ...)
{
    TRACE("ios_open(path=%s, flags=%d)", path, flags);
    
    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }
    
    return open(path, flags, mode);
}

/* Close file */
int ios_close(int fd)
{
    return close(fd);
}

/* Read from file */
ssize_t ios_read(int fd, void *buf, size_t count)
{
    return read(fd, buf, count);
}

/* Write to file */
ssize_t ios_write(int fd, const void *buf, size_t count)
{
    return write(fd, buf, count);
}

/* Seek file */
off_t ios_lseek(int fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}

/* Stat file */
int ios_stat(const char *path, struct stat *buf)
{
    return stat(path, buf);
}

/* Fstat file */
int ios_fstat(int fd, struct stat *buf)
{
    return fstat(fd, buf);
}

/* Get file size */
off_t ios_file_size(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/* Map file to memory */
void *ios_mmap_file(int fd, off_t offset, size_t len, int prot)
{
    int ios_prot = 0;
    if (prot & IOS_PROT_READ) ios_prot |= PROT_READ;
    if (prot & IOS_PROT_WRITE) ios_prot |= PROT_WRITE;
    if (prot & IOS_PROT_EXEC) ios_prot |= PROT_EXEC;
    
    return mmap(NULL, len, ios_prot, MAP_PRIVATE, fd, offset);
}

/*
 * Networking
 */

/* Socket operations */
int ios_socket(int domain, int type, int protocol)
{
    TRACE("ios_socket(domain=%d, type=%d, protocol=%d)", domain, type, protocol);
    return socket(domain, type, protocol);
}

int ios_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect(sockfd, addr, addrlen);
}

int ios_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

int ios_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

int ios_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return accept(sockfd, addr, addrlen);
}

ssize_t ios_send(int sockfd, const void *buf, size_t len, int flags)
{
    return send(sockfd, buf, len, flags);
}

ssize_t ios_recv(int sockfd, void *buf, size_t len, int flags)
{
    return recv(sockfd, buf, len, flags);
}

int ios_close_socket(int sockfd)
{
    return close(sockfd);
}

/*
 * Time operations
 */

/* Get current time */
void ios_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    gettimeofday(tv, tz);
}

/* Sleep */
unsigned int ios_sleep(unsigned int seconds)
{
    return sleep(seconds);
}

int ios_usleep(useconds_t usec)
{
    return usleep(usec);
}

/* Get clock time in nanoseconds */
uint64_t ios_clock_gettime_ns(void)
{
    static mach_timebase_info_data_t timebase;
    static bool timebase_initialized = false;
    
    if (!timebase_initialized) {
        mach_timebase_info(&timebase);
        timebase_initialized = true;
    }
    
    uint64_t time = mach_absolute_time();
    return time * timebase.numer / timebase.denom;
}

/*
 * Environment
 */

/* Get environment variable */
char *ios_getenv(const char *name)
{
    return getenv(name);
}

/* Set environment variable */
int ios_setenv(const char *name, const char *value, int overwrite)
{
    return setenv(name, value, overwrite);
}

/* Get all environment variables */
char **ios_environ(void)
{
    return environ;
}

/*
 * Dynamic linking
 */

/* Load dynamic library */
void *ios_dlopen(const char *filename, int flag)
{
    TRACE("ios_dlopen(filename=%s, flag=%d)", filename ? filename : "(null)", flag);
    return dlopen(filename, flag);
}

/* Get symbol from library */
void *ios_dlsym(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

/* Close library */
int ios_dlclose(void *handle)
{
    return dlclose(handle);
}

/* Get error string */
const char *ios_dlerror(void)
{
    return dlerror();
}

/*
 * Atomics
 */

/* Memory barrier */
void ios_mb(void)
{
    __sync_synchronize();
}

/* Atomic operations */
int ios_atomic_add(int *ptr, int val)
{
    return __sync_add_and_fetch(ptr, val);
}

int ios_atomic_sub(int *ptr, int val)
{
    return __sync_sub_and_fetch(ptr, val);
}

int ios_atomic_inc(int *ptr)
{
    return __sync_add_and_fetch(ptr, 1);
}

int ios_atomic_dec(int *ptr)
{
    return __sync_sub_and_fetch(ptr, 1);
}

int ios_atomic_cas(int *ptr, int old_val, int new_val)
{
    return __sync_val_compare_and_swap(ptr, old_val, new_val);
}

void *ios_atomic_ptr_cas(void **ptr, void *old_val, void *new_val)
{
    return __sync_val_compare_and_swap(ptr, old_val, new_val);
}

/*
 * CPU information
 */

/* Get processor count */
int ios_get_cpu_count(void)
{
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

/* Get current CPU ID */
int ios_get_current_cpu(void)
{
    /* iOS doesn't expose CPU affinity in the same way */
    return 0;
}

/* Get processor affinity */
int ios_get_affinity(int pid, size_t *cpuset, size_t setsize)
{
    /* Simplified implementation */
    memset(cpuset, 0xFF, setsize);
    return 0;
}

/* Set processor affinity */
int ios_set_affinity(int pid, size_t *cpuset, size_t setsize)
{
    /* iOS doesn't support processor affinity setting */
    return -1;
}

/*
 * Initialization
 */

/* Initialize iOS syscall layer */
int ios_syscall_init(void)
{
    TRACE("ios_syscall_init()");
    
    if (g_initialized) {
        return 0;
    }
    
    /* Initialize TLS key */
    if (!g_tls_key_init) {
        pthread_key_create(&g_tls_key, NULL);
        g_tls_key_init = true;
    }
    
    g_initialized = true;
    
    TRACE("iOS syscall layer initialized (jailbroken=%s)", 
          IOS_JAILBROKEN ? "yes" : "no");
    
    return 0;
}

/* Cleanup iOS syscall layer */
void ios_syscall_cleanup(void)
{
    TRACE("ios_syscall_cleanup()");
    
    if (g_jit_region) {
        munmap(g_jit_region, IOS_JIT_REGION_SIZE);
        g_jit_region = NULL;
        g_jit_used = 0;
    }
    
    if (g_tls_key_init) {
        pthread_key_delete(g_tls_key);
        g_tls_key_init = false;
    }
    
    g_initialized = false;
}
