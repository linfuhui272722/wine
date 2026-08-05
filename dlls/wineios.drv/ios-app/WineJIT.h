/*
 * WineJIT - JIT Compilation Support for Wine on iOS
 *
 * Copyright 2024 Wine Project
 *
 * This module provides JIT (Just-In-Time) compilation support for Wine
 * running on jailbroken iOS devices. It enables dynamic code generation
 * and memory execution that would otherwise be blocked by iOS sandboxing.
 */

#import <Foundation/Foundation.h>
#import <mach/mach.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * WineJIT
 * 
 * Provides JIT compilation capabilities for Wine on iOS.
 * On jailbroken devices, this enables:
 * - RWX memory allocation
 * - Dynamic code generation
 * - Function trampolines
 * - Code patching
 */
@interface WineJIT : NSObject

/**
 * Check if JIT is currently enabled
 */
@property (class, nonatomic, readonly, getter=isJITEnabled) BOOL jitEnabled;

/**
 * Initialize JIT subsystem
 * @return YES on success
 */
+ (BOOL)initializeJIT;

/**
 * Shutdown JIT subsystem
 */
+ (void)shutdownJIT;

/**
 * Allocate executable memory for JIT code
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
+ (nullable void *)allocateExecMemory:(size_t)size;

/**
 * Free executable memory
 * @param memory Pointer to memory
 * @param size Size of memory
 */
+ (void)freeExecMemory:(void *)memory size:(size_t)size;

/**
 * Make memory executable
 * @param address Starting address
 * @param size Size of memory region
 * @return YES on success
 */
+ (BOOL)makeMemoryExecutable:(void *)address size:(size_t)size;

/**
 * Make memory writable (for patching)
 * @param address Starting address
 * @param size Size of memory region
 * @return YES on success
 */
+ (BOOL)makeMemoryWritable:(void *)address size:(size_t)size;

/**
 * Execute JIT-compiled code
 * @param code Pointer to code
 * @param args Arguments to pass
 * @return Result from executed code
 */
+ (uintptr_t)executeCode:(void (^)(void))code;

/**
 * Generate function prologue for trampoline
 * @param target Target function address
 * @param scratch Scratch buffer for code
 * @param scratchSize Size of scratch buffer
 * @return Actual size of generated prologue
 */
+ (size_t)generatePrologue:(void *)target scratch:(void *)scratch scratchSize:(size_t)scratchSize;

/**
 * Generate ARM64 branch instruction
 * @param from Source address
 * @param to Target address
 * @return ARM64 branch instruction encoding
 */
+ (uint32_t)arm64BranchFrom:(const void *)from to:(const void *)to;

/**
 * Generate ARM64 branch with link instruction
 * @param from Source address
 * @param to Target address
 * @return ARM64 branch with link instruction encoding
 */
+ (uint32_t)arm64BranchWithLinkFrom:(const void *)from to:(const void *)to;

/**
 * Flush instruction cache
 * @param address Starting address
 * @param size Size to flush
 */
+ (void)flushInstructionCache:(void *)address size:(size_t)size;

/**
 * Register a JIT-compiled function with Wine
 * @param address Address of compiled code
 * @param size Size of code
 * @param name Optional name for debugging
 */
+ (void)registerJITCode:(void *)address size:(size_t)size name:(nullable const char *)name;

/**
 * Unregister JIT-compiled function
 * @param address Address of compiled code
 */
+ (void)unregisterJITCode:(void *)address;

/**
 * Get current JIT statistics
 */
+ (NSDictionary<NSString *, NSNumber *> *)jitStatistics;

@end

NS_ASSUME_NONNULL_END
