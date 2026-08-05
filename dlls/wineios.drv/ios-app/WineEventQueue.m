/*
 * WineEventQueue Implementation
 *
 * Copyright 2024 Wine Project
 */

#import "WineEventQueue.h"
#include <mach/mach_time.h>
#include <pthread.h>

// Maximum size for event data
#define MAX_EVENT_DATA_SIZE 4096

@interface WineEventEntry : NSObject
@property (nonatomic, strong) NSData *data;
@property (nonatomic, assign) uint64_t timestamp;
@end

@implementation WineEventEntry
@end

@interface WineEventQueue ()
{
    NSMutableArray<WineEventEntry *> *_events;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    NSInteger _capacity;
    BOOL _shutdown;
}

@property (nonatomic, strong) NSMutableArray<WineEventEntry *> *events;

@end

@implementation WineEventQueue

@synthesize capacity = _capacity;
@synthesize shuttingDown = _shutdown;

- (instancetype)initWithCapacity:(NSInteger)capacity
{
    self = [super init];
    if (self) {
        _capacity = capacity > 0 ? capacity : 256;
        _events = [NSMutableArray arrayWithCapacity:_capacity];
        _shutdown = NO;
        
        pthread_mutex_init(&_mutex, NULL);
        pthread_cond_init(&_cond, NULL);
    }
    return self;
}

- (void)dealloc
{
    [self signalShutdown];
    
    pthread_cond_destroy(&_cond);
    pthread_mutex_destroy(&_mutex);
    // ARC handles super dealloc automatically
}

- (NSInteger)count
{
    pthread_mutex_lock(&_mutex);
    NSInteger c = self.events.count;
    pthread_mutex_unlock(&_mutex);
    return c;
}

- (BOOL)pushEvent:(const void *)data size:(size_t)size
{
    if (!data || size == 0 || size > MAX_EVENT_DATA_SIZE) {
        return NO;
    }
    
    pthread_mutex_lock(&_mutex);
    
    if (_shutdown) {
        pthread_mutex_unlock(&_mutex);
        return NO;
    }
    
    // Remove oldest if at capacity
    while (self.events.count >= _capacity) {
        [self.events removeObjectAtIndex:0];
    }
    
    // Create event entry
    WineEventEntry *entry = [[WineEventEntry alloc] init];
    entry.data = [NSData dataWithBytes:data length:size];
    entry.timestamp = mach_absolute_time();
    
    [self.events addObject:entry];
    
    // Signal waiting threads
    pthread_cond_signal(&_cond);
    
    pthread_mutex_unlock(&_mutex);
    
    return YES;
}

- (BOOL)popEvent:(void *)data size:(size_t *)size timeoutMS:(NSInteger)timeoutMS
{
    if (!data || !size) {
        return NO;
    }
    
    pthread_mutex_lock(&_mutex);
    
    while (self.events.count == 0 && !_shutdown) {
        if (timeoutMS < 0) {
            // Infinite wait
            pthread_cond_wait(&_cond, &_mutex);
        } else {
            // Timed wait
            struct timespec ts;
            ts.tv_sec = timeoutMS / 1000;
            ts.tv_nsec = (timeoutMS % 1000) * 1000000;
            
            int ret = pthread_cond_timedwait(&_cond, &_mutex, &ts);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&_mutex);
                return NO;
            }
        }
    }
    
    if (_shutdown && self.events.count == 0) {
        pthread_mutex_unlock(&_mutex);
        return NO;
    }
    
    // Get event
    WineEventEntry *entry = self.events.firstObject;
    [self.events removeObjectAtIndex:0];
    
    pthread_mutex_unlock(&_mutex);
    
    // Copy data to output buffer
    size_t copySize = MIN(entry.data.length, *size);
    memcpy(data, entry.data.bytes, copySize);
    *size = copySize;
    
    return YES;
}

- (BOOL)peekEvent:(void *)data size:(size_t *)size
{
    if (!data || !size) {
        return NO;
    }
    
    pthread_mutex_lock(&_mutex);
    
    if (self.events.count == 0) {
        pthread_mutex_unlock(&_mutex);
        return NO;
    }
    
    WineEventEntry *entry = self.events.firstObject;
    
    size_t copySize = MIN(entry.data.length, *size);
    memcpy(data, entry.data.bytes, copySize);
    *size = copySize;
    
    pthread_mutex_unlock(&_mutex);
    
    return YES;
}

- (void)clear
{
    pthread_mutex_lock(&_mutex);
    [self.events removeAllObjects];
    pthread_mutex_unlock(&_mutex);
}

- (void)signalShutdown
{
    pthread_mutex_lock(&_mutex);
    _shutdown = YES;
    pthread_cond_broadcast(&_cond);
    pthread_mutex_unlock(&_mutex);
}

@end
