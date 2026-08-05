/*
 * WineEventQueue - Event queue for Wine communication
 *
 * Copyright 2024 Wine Project
 */

#import <Foundation/Foundation.h>
#import <pthread.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * WineEventQueue
 * 
 * Thread-safe event queue for communication between iOS UI
 * and the Wine Windows environment.
 */
@interface WineEventQueue : NSObject

/** Event queue capacity */
@property (nonatomic, assign, readonly) NSInteger capacity;

/** Current event count */
@property (nonatomic, assign, readonly) NSInteger count;

/** Whether the queue is shutting down */
@property (nonatomic, assign, readonly, getter=isShuttingDown) BOOL shuttingDown;

/**
 * Create a new event queue
 * @param capacity Maximum number of events
 */
- (instancetype)initWithCapacity:(NSInteger)capacity;

/**
 * Push an event to the queue
 * @param data Event data
 * @param size Size of event data
 * @return YES if event was queued
 */
- (BOOL)pushEvent:(const void *)data size:(size_t)size;

/**
 * Pop an event from the queue
 * @param data Buffer to store event data
 * @param size Pointer to size (input: buffer size, output: actual size)
 * @param timeoutMS Timeout in milliseconds (-1 for infinite)
 * @return YES if event was retrieved
 */
- (BOOL)popEvent:(void *)data size:(size_t *)size timeoutMS:(NSInteger)timeoutMS;

/**
 * Peek at the next event without removing it
 * @param data Buffer to store event data
 * @param size Pointer to size (input: buffer size, output: actual size)
 * @return YES if an event is available
 */
- (BOOL)peekEvent:(void *)data size:(size_t *)size;

/**
 * Clear all events from the queue
 */
- (void)clear;

/**
 * Signal shutdown to wake up waiting threads
 */
- (void)signalShutdown;

@end

NS_ASSUME_NONNULL_END
