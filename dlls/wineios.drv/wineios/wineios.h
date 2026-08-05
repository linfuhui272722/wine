/*
 * Wine iOS Main Header
 *
 * Copyright 2024 Wine Project
 *
 * Main header that includes all Wine iOS components.
 */

#ifndef __WINE_IOS_H
#define __WINE_IOS_H

/* Core components */
#include "ios_syscalls.h"
#include "ios_graphics.h"
#include "ios_pe.h"

/* Version information */
#define WINEIOS_VERSION_MAJOR 1
#define WINEIOS_VERSION_MINOR 0
#define WINEIOS_VERSION_PATCH 0

/*
 * Wine iOS initialization
 */

/* Initialize all Wine iOS components */
int wineios_init(void);

/* Cleanup all Wine iOS components */
void wineios_cleanup(void);

/*
 * Wine iOS information
 */

/* Get version string */
const char *wineios_get_version(void);

/* Get capabilities */
uint32_t wineios_get_capabilities(void);

/* Check if jailbroken */
bool wineios_is_jailbroken(void);

/* Check if JIT is available */
bool wineios_has_jit_support(void);

/*
 * Wine iOS configuration
 */

/* Set wine prefix path */
int wineios_set_prefix(const char *path);

/* Get wine prefix path */
const char *wineios_get_prefix(void);

/* Set debug level */
void wineios_set_debug(const char *channels);

/*
 * Wine iOS execution
 */

/* Execute a Windows program */
int wineios_exec(const char *program, const char *args);

/* Run a Windows program and wait for completion */
int wineios_run(const char *program, const char *args);

/*
 * Wine iOS services
 */

/* Start Wine server */
int wineios_start_server(void);

/* Stop Wine server */
int wineios_stop_server(void);

/*
 * Wine iOS capabilities
 */
#define WINEIOS_CAP_JIT          0x00000001  /* JIT compilation */
#define WINEIOS_CAP_METAL        0x00000002  /* Metal graphics */
#define WINEIOS_CAP_OPENGL_ES    0x00000004  /* OpenGL ES */
#define WINEIOS_CAP_MULTITHREAD 0x00000008  /* Multithreading */
#define WINEIOS_CAP_NETWORK      0x00000010  /* Network support */
#define WINEIOS_CAP_FILESYSTEM   0x00000020  /* Full filesystem access */
#define WINEIOS_CAP_JAILBREAK    0x80000000  /* Device is jailbroken */

#endif /* __WINE_IOS_H */
