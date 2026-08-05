/*
 * WineBridge Implementation
 *
 * Copyright 2024 Wine Project
 */

#import "WineBridge.h"
#import "WineEventQueue.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

// Wine debug channel (stubbed)
#define WINE_TRACE(...) do {} while(0)
#define WINE_WARN(...) do {} while(0)
#define WINE_ERR(...) do {} while(0)

/* Unix socket path for Wine communication */
#define WINE_BRIDGE_SOCKET_PATH "/tmp/.wine_ios_bridge"

static WineBridge *_sharedBridge = nil;

@interface WineBridge ()
{
    int _socketFd;
    dispatch_queue_t _sendQueue;
    dispatch_queue_t _receiveQueue;
    WineEventQueue *_eventQueue;
    BOOL _connected;
    
    void (^_surfaceUpdateCallback)(NSInteger, void *, NSInteger, NSInteger);
}

@property (nonatomic, strong) dispatch_queue_t sendQueue;
@property (nonatomic, strong) dispatch_queue_t receiveQueue;
@property (nonatomic, strong) WineEventQueue *eventQueue;

@end

@implementation WineBridge

@synthesize connected = _connected;

+ (WineBridge *)sharedBridge
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedBridge = [[WineBridge alloc] init];
    });
    return _sharedBridge;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _socketFd = -1;
        _connected = NO;
        _surfaceUpdateCallback = nil;
        
        _sendQueue = dispatch_queue_create("com.wine.bridge.send", DISPATCH_QUEUE_SERIAL);
        _receiveQueue = dispatch_queue_create("com.wine.bridge.receive", DISPATCH_QUEUE_SERIAL);
        _eventQueue = [[WineEventQueue alloc] initWithCapacity:1024];
    }
    return self;
}

- (void)dealloc
{
    [self shutdown];
    // ARC handles super dealloc automatically
}

- (BOOL)initialize
{
    if (_connected) {
        WINE_TRACE("Bridge already connected\n");
        return YES;
    }
    
    WINE_TRACE("Initializing Wine bridge\n");
    
    // Try to connect to Wine socket
    _socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socketFd < 0) {
        WINE_ERR("Failed to create socket: %s\n", strerror(errno));
        return NO;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, WINE_BRIDGE_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(_socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        WINE_WARN("Failed to connect to Wine bridge socket: %s\n", strerror(errno));
        WINE_WARN("Wine may not be running - bridge will operate in local mode\n");
        // Don't fail, bridge will work in local mode
        close(_socketFd);
        _socketFd = -1;
        _connected = YES;  // Consider "connected" even without socket
        return YES;
    }
    
    // Set non-blocking
    int flags = fcntl(_socketFd, F_GETFL, 0);
    fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK);
    
    // Start receive thread
    dispatch_async(_receiveQueue, ^{
        [self receiveLoop];
    });
    
    _connected = YES;
    WINE_TRACE("Wine bridge connected\n");
    
    return YES;
}

- (void)shutdown
{
    if (!_connected) return;
    
    WINE_TRACE("Shutting down Wine bridge\n");
    
    _connected = NO;
    [_eventQueue signalShutdown];
    
    if (_socketFd >= 0) {
        close(_socketFd);
        _socketFd = -1;
    }
}

- (void)receiveLoop
{
    uint8_t buffer[8192];
    
    while (_connected) {
        ssize_t n = recv(_socketFd, buffer, sizeof(buffer), 0);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);  // 10ms
                continue;
            }
            WINE_ERR("Receive error: %s\n", strerror(errno));
            break;
        }
        
        if (n == 0) {
            WINE_TRACE("Connection closed\n");
            break;
        }
        
        // Process received message
        [self processReceivedData:buffer size:n];
    }
    
    _connected = NO;
}

- (void)processReceivedData:(const void *)data size:(size_t)size
{
    // Parse Wine messages
    const uint8_t *ptr = data;
    
    while (size >= 8) {
        uint32_t type = *(uint32_t *)ptr;
        uint32_t msgSize = *(uint32_t *)(ptr + 4);
        
        if (msgSize > size) break;
        
        const void *payload = ptr + 8;
        size_t payloadSize = msgSize - 8;
        
        switch (type) {
            case 0x01:  // Surface update request
                if (payloadSize >= sizeof(NSInteger)) {
                    NSInteger hwnd = *(NSInteger *)payload;
                    if (_surfaceUpdateCallback) {
                        // Request surface data from Wine
                        // This would trigger the callback when data is ready
                    }
                }
                break;
                
            default:
                WINE_TRACE("Received unknown message type: 0x%x\n", type);
                break;
        }
        
        ptr += msgSize;
        size -= msgSize;
    }
}

#pragma mark - Event Posting

- (void)postTouchEvent:(const void *)data size:(size_t)size forHwnd:(NSInteger)hwnd
{
    if (!_connected) return;
    
    dispatch_async(_sendQueue, ^{
        [self sendEvent:data size:size type:WineEventTypeTouch hwnd:hwnd];
    });
}

- (void)postKeyEvent:(UIKey *)keyCode modifiers:(UIKeyModifierFlags)modifiers forHwnd:(NSInteger)hwnd
{
    if (!_connected) return;
    
    typedef struct {
        uint32_t keyCode;
        uint32_t modifiers;
    } KeyEvent;
    
    KeyEvent event = {
        .keyCode = keyCode.keyCode,
        .modifiers = modifiers
    };
    
    dispatch_async(_sendQueue, ^{
        [self sendEvent:&event size:sizeof(event) type:WineEventTypeKey hwnd:hwnd];
    });
}

- (void)postCharacterEvent:(unichar)character modifiers:(UIKeyModifierFlags)modifiers forHwnd:(NSInteger)hwnd
{
    if (!_connected) return;
    
    typedef struct {
        unichar character;
        uint32_t modifiers;
    } CharEvent;
    
    CharEvent event = {
        .character = character,
        .modifiers = modifiers
    };
    
    dispatch_async(_sendQueue, ^{
        [self sendEvent:&event size:sizeof(event) type:WineEventTypeCharacter hwnd:hwnd];
    });
}

- (void)postMouseEvent:(CGFloat)x y:(CGFloat)y button:(NSInteger)button down:(BOOL)down forHwnd:(NSInteger)hwnd
{
    if (!_connected) return;
    
    typedef struct {
        int32_t x;
        int32_t y;
        int32_t button;
        BOOL down;
    } MouseEvent;
    
    MouseEvent event = {
        .x = (int32_t)(x * [UIScreen mainScreen].scale),
        .y = (int32_t)(y * [UIScreen mainScreen].scale),
        .button = (int32_t)button,
        .down = down
    };
    
    dispatch_async(_sendQueue, ^{
        [self sendEvent:&event size:sizeof(event) type:WineEventTypeMouse hwnd:hwnd];
    });
}

- (void)postMouseWheelEvent:(CGFloat)deltaX deltaY:(CGFloat)deltaY forHwnd:(NSInteger)hwnd
{
    if (!_connected) return;
    
    typedef struct {
        int32_t deltaX;
        int32_t deltaY;
    } WheelEvent;
    
    WheelEvent event = {
        .deltaX = (int32_t)(deltaX * 120),  // WHEEL_DELTA = 120
        .deltaY = (int32_t)(deltaY * 120)
    };
    
    dispatch_async(_sendQueue, ^{
        [self sendEvent:&event size:sizeof(event) type:WineEventTypeMouseWheel hwnd:hwnd];
    });
}

- (void)postDisplayChangeEvent:(uint32_t)displayId width:(uint32_t)width height:(uint32_t)height scale:(CGFloat)scale
{
    typedef struct {
        uint32_t displayId;
        uint32_t width;
        uint32_t height;
        uint32_t scale;
    } DisplayEvent;
    
    DisplayEvent event = {
        .displayId = displayId,
        .width = width,
        .height = height,
        .scale = (uint32_t)(scale * 1000)
    };
    
    [self sendEvent:&event size:sizeof(event) type:WineEventTypeDisplayChange hwnd:0];
}

- (void)postOrientationChangeEvent:(UIInterfaceOrientation)orientation
{
    typedef struct {
        uint32_t orientation;
    } OrientEvent;
    
    OrientEvent event = {
        .orientation = (uint32_t)orientation
    };
    
    [self sendEvent:&event size:sizeof(event) type:WineEventTypeOrientation hwnd:0];
}

- (void)postLifecycleEvent:(WineEventType)eventType
{
    [self sendEvent:NULL size:0 type:eventType hwnd:0];
}

#pragma mark - Surface Updates

- (void)requestSurfaceUpdate:(NSInteger)hwnd
{
    typedef struct {
        NSInteger hwnd;
    } SurfaceRequest;
    
    SurfaceRequest request = { .hwnd = hwnd };
    [self sendEvent:&request size:sizeof(request) type:0xFF hwnd:hwnd];
}

- (void)setSurfaceUpdateCallback:(void (^)(NSInteger, void *, NSInteger, NSInteger))callback
{
    _surfaceUpdateCallback = callback;
}

#pragma mark - Raw Messaging

- (BOOL)sendRawMessage:(uint32_t)type data:(const void *)data size:(size_t)size
{
    return [self sendEvent:data size:size type:type hwnd:0];
}

- (BOOL)sendEvent:(const void *)data size:(size_t)size type:(WineEventType)type hwnd:(NSInteger)hwnd
{
    if (!_connected) return NO;
    
    if (_socketFd < 0) {
        // Local mode - queue event for later
        return [_eventQueue pushEvent:data size:size];
    }
    
    // Calculate total message size
    size_t totalSize = 8 + size;  // header + payload
    
    uint8_t *message = malloc(totalSize);
    if (!message) return NO;
    
    // Build message header
    *(uint32_t *)message = (uint32_t)type;
    *(uint32_t *)(message + 4) = (uint32_t)totalSize;
    
    // Copy payload
    if (data && size > 0) {
        memcpy(message + 8, data, size);
    }
    
    // Send message
    ssize_t sent = send(_socketFd, message, totalSize, 0);
    free(message);
    
    if (sent < 0) {
        WINE_WARN("Failed to send event: %s\n", strerror(errno));
        return NO;
    }
    
    return YES;
}

@end
