/*
 * iOS Syscall Bridge
 *
 * Copyright 2024 Wine Project
 *
 * Maps Unix/Linux syscalls to iOS equivalents.
 * Provides the foundation for Wine running on iOS.
 */

#ifndef __WINE_IOS_SYSCALLS_H
#define __WINE_IOS_SYSCALLS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdlib.h>

/* iOS-specific definitions */
#define IOS_JAILBROKEN    (getenv("JAILBREAK") != NULL)
#define IOS_SANDBOXED     (!IOS_JAILBROKEN)

/* Memory protection flags for iOS */
#define IOS_PROT_NONE     0x0
#define IOS_PROT_READ     0x1
#define IOS_PROT_WRITE    0x2
#define IOS_PROT_EXEC     0x4
#define IOS_PROT_RWX     (IOS_PROT_READ | IOS_PROT_WRITE | IOS_PROT_EXEC)

/* Memory mapping flags for iOS */
#define IOS_MAP_ANONYMOUS 0x1000
#define IOS_MAP_PRIVATE   0x0002
#define IOS_MAP_SHARED   0x0001
#define IOS_MAP_FIXED    0x0010
#define IOS_MAP_JIT      0x0800  /* JIT memory region */

/* iOS-specific memory regions */
#define IOS_JIT_REGION_BASE     0x1000000000ULL
#define IOS_JIT_REGION_SIZE      (64 * 1024 * 1024)  /* 64MB */

/*
 * Memory management
 */

/* Allocate RWX memory for JIT code */
void *ios_alloc_jit_memory(size_t size);

/* Free JIT memory */
int ios_free_jit_memory(void *addr, size_t size);

/* Map memory with specified protection */
void *ios_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

/* Unmap memory */
int ios_munmap(void *addr, size_t len);

/* Change memory protection */
int ios_mprotect(void *addr, size_t len, int prot);

/* Flush instruction cache */
void ios_icache_sync(void *addr, size_t len);

/*
 * Process management
 */

/* Create a new process */
pid_t ios_fork(void);

/* Execute a program */
int ios_execve(const char *path, char *const argv[], char *const envp[]);

/* Wait for process */
pid_t ios_waitpid(pid_t pid, int *status, int options);

/* Get current process ID */
pid_t ios_getpid(void);

/* Get current thread ID */
uint64_t ios_gettid(void);

/* Get parent process ID */
pid_t ios_getppid(void);

/* Exit process */
__attribute__((noreturn)) void ios__exit(int status);

/*
 * Thread management
 */

/* Create a new thread */
int ios_thread_create(uint64_t *thread, void *stack, size_t stack_size, 
                      void (*entry)(void *), void *arg);

/* Terminate current thread */
__attribute__((noreturn)) void ios_thread_exit(void *retval);

/* Join a thread */
int ios_thread_join(uint64_t thread, void **retval);

/* Set thread name */
int ios_thread_setname(const char *name);

/* Get thread name */
int ios_thread_getname(char *name, size_t len);

/*
 * Synchronization primitives
 */

/* Mutex operations */
int ios_mutex_init(void **mutex);
int ios_mutex_lock(void *mutex);
int ios_mutex_unlock(void *mutex);
int ios_mutex_destroy(void *mutex);

/* Semaphore operations */
int ios_sem_init(void **sem, int value);
int ios_sem_wait(void *sem);
int ios_sem_post(void *sem);
int ios_sem_destroy(void *sem);

/* Condition variable operations */
int ios_cond_init(void **cond);
int ios_cond_wait(void *cond, void *mutex);
int ios_cond_signal(void *cond);
int ios_cond_broadcast(void *cond);
int ios_cond_destroy(void *cond);

/*
 * File operations
 */

/* Open file */
int ios_open(const char *path, int flags, ...);

/* Close file */
int ios_close(int fd);

/* Read from file */
ssize_t ios_read(int fd, void *buf, size_t count);

/* Write to file */
ssize_t ios_write(int fd, const void *buf, size_t count);

/* Seek file */
off_t ios_lseek(int fd, off_t offset, int whence);

/* Stat file */
int ios_stat(const char *path, struct stat *buf);

/* Fstat file */
int ios_fstat(int fd, struct stat *buf);

/* Get file size */
off_t ios_file_size(int fd);

/* Map file to memory */
void *ios_mmap_file(int fd, off_t offset, size_t len, int prot);

/*
 * Networking
 */

/* Socket operations */
int ios_socket(int domain, int type, int protocol);
int ios_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int ios_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int ios_listen(int sockfd, int backlog);
int ios_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t ios_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t ios_recv(int sockfd, void *buf, size_t len, int flags);
int ios_close_socket(int sockfd);

/*
 * Time operations
 */

/* Get current time */
void ios_gettimeofday(struct timeval *tv, struct timezone *tz);

/* Sleep */
unsigned int ios_sleep(unsigned int seconds);
int ios_usleep(useconds_t usec);

/* Get clock time */
uint64_t ios_clock_gettime_ns(void);

/*
 * Environment
 */

/* Get environment variable */
char *ios_getenv(const char *name);

/* Set environment variable */
int ios_setenv(const char *name, const char *value, int overwrite);

/* Get all environment variables */
char **ios_environ(void);

/*
 * Dynamic linking
 */

/* Load dynamic library */
void *ios_dlopen(const char *filename, int flag);

/* Get symbol from library */
void *ios_dlsym(void *handle, const char *symbol);

/* Close library */
int ios_dlclose(void *handle);

/* Get error string */
const char *ios_dlerror(void);

/*
 * Atomics
 */

/* Memory barrier */
void ios_mb(void);

/* Atomic operations */
int ios_atomic_add(int *ptr, int val);
int ios_atomic_sub(int *ptr, int val);
int ios_atomic_inc(int *ptr);
int ios_atomic_dec(int *ptr);
int ios_atomic_cas(int *ptr, int old_val, int new_val);
void *ios_atomic_ptr_cas(void **ptr, void *old_val, void *new_val);

/*
 * CPU information
 */

/* Get processor count */
int ios_get_cpu_count(void);

/* Get current CPU ID */
int ios_get_current_cpu(void);

/* Get processor affinity */
int ios_get_affinity(int pid, size_t *cpuset, size_t setsize);

/* Set processor affinity */
int ios_set_affinity(int pid, size_t *cpuset, size_t setsize);

/*
 * Initialization
 */

/* Initialize iOS syscall layer */
int ios_syscall_init(void);

/* Cleanup iOS syscall layer */
void ios_syscall_cleanup(void);

#endif /* __WINE_IOS_SYSCALLS_H */
