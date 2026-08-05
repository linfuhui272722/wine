/*
 * WineViewController - UIViewController for Wine Windows
 *
 * Copyright 2024 Wine Project
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

@class WineViewController;

/**
 * WineViewControllerDelegate
 * 
 * Protocol for Wine view controller events
 */
@protocol WineViewControllerDelegate <NSObject>
@optional
- (void)wineViewController:(WineViewController *)controller didReceiveTouchEvent:(UIEvent *)event;
- (void)wineViewController:(WineViewController *)controller didReceiveKeyEvent:(UIKey *)event API_AVAILABLE(ios(13.4));
- (void)wineViewController:(WineViewController *)controller didChangeFrame:(CGRect)frame;
- (void)wineViewControllerRequestsClose:(WineViewController *)controller;
@end

/**
 * Rendering mode for the Wine window
 */
typedef NS_ENUM(NSInteger, WineRenderingMode) {
    WineRenderingModeCoreGraphics,   // 2D rendering using Core Graphics
    WineRenderingModeOpenGL,        // OpenGL ES rendering
    WineRenderingModeMetal,         // Metal rendering (preferred)
    WineRenderingModeSoftware       // Software rendering (fallback)
};

/**
 * WineViewController
 * 
 * A UIViewController subclass that hosts a Wine window.
 * Supports multiple rendering modes and handles touch/keyboard input.
 */
@interface WineViewController : UIViewController

/** The Windows HWND this view represents */
@property (nonatomic, assign, readonly) NSInteger hwnd;

/** Current rendering mode */
@property (nonatomic, assign) WineRenderingMode renderingMode;

/** Delegate for Wine-specific events */
@property (nonatomic, assign, nullable) id<WineViewControllerDelegate> wineDelegate;

/** Whether the view is currently visible */
@property (nonatomic, assign, readonly, getter=isVisible) BOOL visible;

/** Framebuffer surface for rendering */
@property (nonatomic, strong, readonly, nullable) id<CAMetalDrawable> metalDrawable;

/** Wine surface properties */
@property (nonatomic, assign, readonly) CGSize surfaceSize;
@property (nonatomic, assign, readonly) NSInteger surfaceFormat;

/** OpenGL renderbuffer (stored as integer handle) */
@property (nonatomic, assign, readonly) GLuint eaglRenderbuffer;

/** Display link for synchronized rendering */
@property (nonatomic, strong, readonly, nullable) CADisplayLink *displayLink;

/**
 * Initialize with a Windows handle
 * @param hwnd The Windows window handle
 */
- (instancetype)initWithHwnd:(NSInteger)hwnd;

/**
 * Setup the view for a specific rendering mode
 * @param mode The rendering mode to use
 */
- (void)setupWithRenderingMode:(WineRenderingMode)mode;

/**
 * Notify Wine that the view needs redrawing
 */
- (void)setNeedsDisplay;

/**
 * Invalidate the surface and force redraw
 */
- (void)invalidateSurface;

/**
 * Update the window position from Wine
 * @param rect New window rectangle
 */
- (void)updateWindowRect:(CGRect)rect;

/**
 * Set the window surface data
 * @param buffer Pixel data buffer
 * @param stride Bytes per row
 * @param format Pixel format
 */
- (void)setSurfaceData:(void *)buffer stride:(NSInteger)stride format:(NSInteger)format;

/**
 * Enable or disable touch input
 * @param enabled Whether touch input is enabled
 */
- (void)setTouchInputEnabled:(BOOL)enabled;

/**
 * Enable or disable keyboard input
 * @param enabled Whether keyboard input is enabled
 */
- (void)setKeyboardInputEnabled:(BOOL)enabled;

/**
 * Show or hide the keyboard
 * @param show Whether to show the keyboard
 * @param animated Whether to animate the transition
 */
- (void)showKeyboard:(BOOL)show animated:(BOOL)animated;

@end

NS_ASSUME_NONNULL_END
