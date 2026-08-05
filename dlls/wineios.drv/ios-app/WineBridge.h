/*
 * WineBridge - Bridge between iOS and Wine
 *
 * Copyright 2024 Wine Project
 *
 * This module provides the communication bridge between native iOS
 * code and the Wine Windows environment running on the device.
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Event types that can be sent to Wine
 */
typedef NS_ENUM(NSInteger, WineEventType) {
    WineEventTypeTouch,           // Touch/multi-touch input
    WineEventTypeKey,             // Keyboard key press
    WineEventTypeCharacter,        // Character input
    WineEventTypeMouse,           // Mouse event
    WineEventTypeMouseWheel,      // Mouse wheel scroll
    WineEventTypeDisplayChange,   // Display configuration change
    WineEventTypeOrientation,     // Device orientation change
    WineEventTypeMemoryWarning,   // Memory pressure warning
    WineEventTypeAppSuspend,      // Application going to background
    WineEventTypeAppResume,       // Application returning to foreground
    WineEventTypeVolumeButton,    // Volume button press
    WineEventTypePowerButton,     // Power button press
};

/**
 * WineBridge
 * 
 * Singleton class that handles communication between the iOS UI
 * and Wine's Windows environment.
 */
@interface WineBridge : NSObject

/**
 * Shared bridge instance
 */
@property (class, nonatomic, readonly) WineBridge *sharedBridge;

/**
 * Whether the bridge is connected to Wine
 */
@property (nonatomic, assign, readonly, getter=isConnected) BOOL connected;

/**
 * Initialize the bridge
 * @return YES on success
 */
- (BOOL)initialize;

/**
 * Shutdown the bridge
 */
- (void)shutdown;

/**
 * Post a touch event to Wine
 * @param data Event data
 * @param size Size of event data
 * @param hwnd Target window handle
 */
- (void)postTouchEvent:(const void *)data size:(size_t)size forHwnd:(NSInteger)hwnd;

/**
 * Post a keyboard event to Wine
 * @param keyCode Virtual key code
 * @param modifiers Key modifiers
 * @param hwnd Target window handle
 */
- (void)postKeyEvent:(UIKey *)keyCode modifiers:(UIKeyModifierFlags)modifiers forHwnd:(NSInteger)hwnd API_AVAILABLE(ios(13.4));

/**
 * Post a character event to Wine
 * @param character Character
 * @param modifiers Key modifiers
 * @param hwnd Target window handle
 */
- (void)postCharacterEvent:(unichar)character modifiers:(UIKeyModifierFlags)modifiers forHwnd:(NSInteger)hwnd;

/**
 * Post a mouse event to Wine
 * @param x X position
 * @param y Y position
 * @param button Button (0=left, 1=right, 2=middle)
 * @param down Whether button is pressed
 * @param hwnd Target window handle
 */
- (void)postMouseEvent:(CGFloat)x y:(CGFloat)y button:(NSInteger)button down:(BOOL)down forHwnd:(NSInteger)hwnd;

/**
 * Post a mouse wheel event to Wine
 * @param deltaX Horizontal scroll
 * @param deltaY Vertical scroll
 * @param hwnd Target window handle
 */
- (void)postMouseWheelEvent:(CGFloat)deltaX deltaY:(CGFloat)deltaY forHwnd:(NSInteger)hwnd;

/**
 * Post a display change notification
 * @param displayId Display identifier
 * @param width New width
 * @param height New height
 * @param scale New scale factor
 */
- (void)postDisplayChangeEvent:(uint32_t)displayId width:(uint32_t)width height:(uint32_t)height scale:(CGFloat)scale;

/**
 * Post an orientation change notification
 * @param orientation New orientation
 */
- (void)postOrientationChangeEvent:(UIInterfaceOrientation)orientation;

/**
 * Post an application lifecycle event
 * @param eventType Event type
 */
- (void)postLifecycleEvent:(WineEventType)eventType;

/**
 * Request a window surface update
 * @param hwnd Window handle
 */
- (void)requestSurfaceUpdate:(NSInteger)hwnd;

/**
 * Set the callback for surface updates
 * @param callback Callback block
 */
- (void)setSurfaceUpdateCallback:(void (^)(NSInteger hwnd, void *buffer, NSInteger stride, NSInteger format))callback;

/**
 * Send raw data to Wine
 * @param type Message type
 * @param data Data to send
 * @param size Size of data
 * @return YES on success
 */
- (BOOL)sendRawMessage:(uint32_t)type data:(const void *)data size:(size_t)size;

@end

NS_ASSUME_NONNULL_END
