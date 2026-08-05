/*
 * Wine iOS Application Delegate
 *
 * Copyright 2024 Wine Project
 *
 * This is the main entry point for the Wine iOS application
 * that runs on jailbroken iOS devices with JIT support.
 */

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class WineViewController;
@class WineDocumentController;
@class WineMenuViewController;

/**
 * Wine iOS Application Delegate
 * 
 * Manages the application lifecycle and coordinates between
 * the native iOS UI and the Wine Windows environment.
 */
@interface WineAppDelegate : UIResponder <UIApplicationDelegate>

/** Main window for the application */
@property (strong, nonatomic) UIWindow *window;

/** Root view controller for navigation */
@property (strong, nonatomic) UINavigationController *navigationController;

/** Active Wine windows */
@property (strong, nonatomic) NSMutableDictionary<NSNumber *, WineViewController *> *wineWindows;

/** Main Wine view controller */
@property (strong, nonatomic, nullable) WineViewController *mainWineViewController;

/** Menu view controller for selecting executables */
@property (strong, nonatomic, nullable) WineMenuViewController *menuViewController;

/** Documents directory path for Wine */
@property (copy, nonatomic, readonly) NSString *winePrefixPath;

/** Cache directory for Wine temp files */
@property (copy, nonatomic, readonly) NSString *wineCachePath;

/** Documents directory accessible to user */
@property (copy, nonatomic, readonly) NSString *wineDocumentsPath;

/** Whether Wine environment is initialized */
@property (assign, nonatomic, readonly, getter=isWineInitialized) BOOL wineInitialized;

/** Wine process ID when running */
@property (assign, nonatomic) pid_t wineProcessId;

/** Whether JIT is enabled */
@property (assign, nonatomic, readonly, getter=isJitEnabled) BOOL jitEnabled;

+ (instancetype)sharedDelegate;

/**
 * Initialize the Wine environment
 * @return YES on success, NO on failure
 */
- (BOOL)setupWineEnvironment;

/**
 * Start Wine with optional executable path
 * @param executablePath Path to Windows executable, or nil for explorer
 * @return YES if Wine started successfully
 */
- (BOOL)startWine:(nullable NSString *)executablePath;

/**
 * Stop the running Wine process
 */
- (BOOL)stopWine;

/**
 * Launch a Windows executable
 * @param path Path to the executable file
 */
- (void)launchExecutableAtPath:(NSString *)path;

/**
 * Create a new Wine window for a given HWND
 * @param hwnd The window handle
 * @return The created view controller
 */
- (nullable WineViewController *)createWindowForHwnd:(NSInteger)hwnd;

/**
 * Post an event to the Wine event queue
 * @param eventData Event data structure
 * @param size Size of event data
 */
- (void)postEventToWine:(const void *)eventData size:(size_t)size;

/**
 * Get screen scale factor
 * @return Scale factor (1.0, 2.0, or 3.0)
 */
- (CGFloat)screenScale;

/**
 * Get safe area insets
 * @return Safe area rect
 */
- (UIEdgeInsets)safeAreaInsets;

@end

NS_ASSUME_NONNULL_END
