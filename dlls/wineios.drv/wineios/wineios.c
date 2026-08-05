/*
 * Wine iOS Main Implementation
 *
 * Copyright 2024 Wine Project
 *
 * Main entry point that initializes and coordinates all Wine iOS components.
 */

#include "wineios.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define TRACE_WINEIOS 1

#if TRACE_WINEIOS
#define TRACE(fmt, ...) fprintf(stderr, "[wineios] " fmt "\n", ##__VA_ARGS__)
#else
#define TRACE(fmt, ...) do {} while(0)
#endif

#define WARN(fmt, ...) fprintf(stderr, "[wineios:warn] " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "[wineios:err] " fmt "\n", ##__VA_ARGS__)

/* Global state */
static bool g_initialized = false;
static char g_prefix[256] = {0};
static uint32_t g_capabilities = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Wine iOS initialization
 */

int wineios_init(void)
{
    TRACE("wineios_init()");
    
    pthread_mutex_lock(&g_mutex);
    
    if (g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    
    /* Initialize syscall layer */
    if (ios_syscall_init() != 0) {
        ERR("Failed to initialize syscall layer");
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    /* Initialize graphics */
    if (ios_graphics_init() != 0) {
        ERR("Failed to initialize graphics");
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    /* Initialize PE loader */
    if (ios_pe_init() != 0) {
        ERR("Failed to initialize PE loader");
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    /* Set default prefix */
    const char *home = ios_getenv("HOME");
    if (home) {
        snprintf(g_prefix, sizeof(g_prefix) - 1, "%s/wine", home);
    } else {
        strcpy(g_prefix, "/tmp/wine");
    }
    
    /* Determine capabilities */
    g_capabilities = 0;
    
    if (IOS_JAILBROKEN) {
        g_capabilities |= WINEIOS_CAP_JIT;
        g_capabilities |= WINEIOS_CAP_FILESYSTEM;
        g_capabilities |= WINEIOS_CAP_JAILBREAK;
        TRACE("Device is jailbroken - JIT enabled");
    } else {
        WARN("Device is not jailbroken - limited functionality");
    }
    
    g_capabilities |= WINEIOS_CAP_METAL;
    g_capabilities |= WINEIOS_CAP_MULTITHREAD;
    g_capabilities |= WINEIOS_CAP_NETWORK;
    
    g_initialized = true;
    
    pthread_mutex_unlock(&g_mutex);
    
    TRACE("Wine iOS initialized successfully");
    TRACE("Capabilities: 0x%08x", g_capabilities);
    TRACE("Wine prefix: %s", g_prefix);
    
    return 0;
}

void wineios_cleanup(void)
{
    TRACE("wineios_cleanup()");
    
    pthread_mutex_lock(&g_mutex);
    
    if (!g_initialized) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    
    /* Cleanup PE loader */
    ios_pe_cleanup();
    
    /* Cleanup graphics */
    ios_graphics_cleanup();
    
    /* Cleanup syscall layer */
    ios_syscall_cleanup();
    
    g_initialized = false;
    
    pthread_mutex_unlock(&g_mutex);
    
    TRACE("Wine iOS cleanup complete");
}

/*
 * Wine iOS information
 */

const char *wineios_get_version(void)
{
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d",
             WINEIOS_VERSION_MAJOR, WINEIOS_VERSION_MINOR, WINEIOS_VERSION_PATCH);
    return version;
}

uint32_t wineios_get_capabilities(void)
{
    return g_capabilities;
}

bool wineios_is_jailbroken(void)
{
    return IOS_JAILBROKEN;
}

bool wineios_has_jit_support(void)
{
    return (g_capabilities & WINEIOS_CAP_JIT) != 0;
}

/*
 * Wine iOS configuration
 */

int wineios_set_prefix(const char *path)
{
    if (!path) return -1;
    
    pthread_mutex_lock(&g_mutex);
    strncpy(g_prefix, path, sizeof(g_prefix) - 1);
    pthread_mutex_unlock(&g_mutex);
    
    ios_setenv("WINEPREFIX", path, 1);
    
    return 0;
}

const char *wineios_get_prefix(void)
{
    return g_prefix;
}

void wineios_set_debug(const char *channels)
{
    if (channels) {
        ios_setenv("WINEDEBUG", channels, 1);
    }
}

/*
 * Wine iOS execution
 */

int wineios_exec(const char *program, const char *args)
{
    TRACE("wineios_exec(%s, %s)", program, args ? args : "(null)");
    
    if (!g_initialized) {
        if (wineios_init() != 0) {
            return -1;
        }
    }
    
    /* Use PE loader to execute program */
    char *argv[] = { (char *)program, (char *)args, NULL };
    return ios_pe_create_process(program, 1, argv, ios_environ());
}

int wineios_run(const char *program, const char *args)
{
    TRACE("wineios_run(%s, %s)", program, args ? args : "(null)");
    
    int result = wineios_exec(program, args);
    
    /* For now, just return result */
    /* Full implementation would wait for process completion */
    
    return result;
}

/*
 * Wine iOS services
 */

int wineios_start_server(void)
{
    TRACE("wineios_start_server()");
    
    /* Would start Wine server process here */
    /* Wine server handles IPC between Wine processes */
    
    return 0;
}

int wineios_stop_server(void)
{
    TRACE("wineios_stop_server()");
    
    /* Would stop Wine server process here */
    
    return 0;
}
