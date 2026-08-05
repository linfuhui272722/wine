/*
 * Wine iOS Application Delegate Implementation
 *
 * Copyright 2024 Wine Project
 *
 * Main entry point for Wine iOS application.
 * Integrates Wine components with iOS UI.
 */

#import "WineAppDelegate.h"
#import "WineViewController.h"
#import "WineEventQueue.h"
#import "WineJIT.h"
#import "WineBridge.h"

// Wine iOS headers
#include "wineios.h"
#include "ios_syscalls.h"
#include "ios_graphics.h"
#include "ios_pe.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>
#include <spawn.h>
#include <unistd.h>
#include <libgen.h>

// Wine debug channel
#define WINE_TRACE(...) do {} while(0)
#define WINE_WARN(...) do {} while(0)
#define WINE_ERR(...) do {} while(0)

@interface WineAppDelegate ()
{
    WineEventQueue *_eventQueue;
    dispatch_queue_t _wineDispatchQueue;
    BOOL _initialized;
    pid_t _winePid;
}

@property (nonatomic, strong) WineEventQueue *eventQueue;
@property (nonatomic, strong) dispatch_queue_t wineDispatchQueue;

@end

@implementation WineAppDelegate

+ (instancetype)sharedDelegate
{
    return (WineAppDelegate *)[UIApplication sharedApplication].delegate;
}

#pragma mark - Application Lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    WINE_TRACE("Wine iOS application launching\n");
    
    // Initialize Wine iOS components
    [self initializeWineiOS];
    
    // Create main window
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    
    // Initialize Wine windows dictionary
    self.wineWindows = [NSMutableDictionary dictionary];
    
    // Create event queue for Wine communication
    self.eventQueue = [[WineEventQueue alloc] initWithCapacity:256];
    
    // Create dispatch queue for Wine operations
    self.wineDispatchQueue = dispatch_queue_create("com.wine.ios.dispatch", DISPATCH_QUEUE_SERIAL);
    
    // Create Wine view controller as main view
    WineViewController *wineVC = [[WineViewController alloc] initWithHwnd:1];
    wineVC.view.backgroundColor = [UIColor blackColor];
    
    // Create navigation controller
    self.navigationController = [[UINavigationController alloc] initWithRootViewController:wineVC];
    self.navigationController.navigationBarHidden = YES;
    
    self.window.rootViewController = self.navigationController;
    [self.window makeKeyAndVisible];
    
    // Store reference to main Wine view controller
    self.mainWineViewController = wineVC;
    
    // Show Wine iOS info
    [self showWineiOSInfo];
    
    // Initialize Wine environment
    dispatch_async(self.wineDispatchQueue, ^{
        [self setupWineEnvironment];
    });
    
    // Register for memory warnings
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(didReceiveMemoryWarning:)
                                                 name:UIApplicationDidReceiveMemoryWarningNotification
                                               object:nil];
    
    // Register for device orientation changes
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(didChangeOrientation:)
                                                 name:UIApplicationDidChangeStatusBarOrientationNotification
                                               object:nil];
    
    return YES;
}

- (void)initializeWineiOS
{
    WINE_TRACE("Initializing Wine iOS components\n");
    
    // Initialize Wine iOS
    int result = wineios_init();
    if (result != 0) {
        WINE_WARN("Wine iOS initialization returned %d\n", result);
    } else {
        WINE_TRACE("Wine iOS initialized successfully\n");
        WINE_TRACE("Version: %s\n", wineios_get_version());
        WINE_TRACE("Capabilities: 0x%08x\n", wineios_get_capabilities());
        WINE_TRACE("Jailbroken: %s\n", wineios_is_jailbroken() ? "yes" : "no");
        WINE_TRACE("JIT support: %s\n", wineios_has_jit_support() ? "yes" : "no");
        WINE_TRACE("Wine prefix: %s\n", wineios_get_prefix());
    }
    
    // Initialize Wine JIT
    [WineJIT initializeJIT];
}

- (void)showWineiOSInfo
{
    // Get device info
    UIDevice *device = [UIDevice currentDevice];
    struct utsname systemInfo;
    uname(&systemInfo);
    
    NSString *deviceName = device.name;
    NSString *systemVersion = device.systemVersion;
    NSString *model = device.model;
    NSString *deviceInfo = [NSString stringWithFormat:@"%@ (%@)", model, systemVersion];
    
    // Create info overlay
    UILabel *infoLabel = [[UILabel alloc] init];
    infoLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.7];
    infoLabel.textColor = [UIColor whiteColor];
    infoLabel.font = [UIFont systemFontOfSize:12];
    infoLabel.numberOfLines = 0;
    infoLabel.textAlignment = NSTextAlignmentLeft;
    infoLabel.layer.cornerRadius = 8;
    infoLabel.layer.masksToBounds = YES;
    infoLabel.translatesAutoresizingMaskIntoConstraints = NO;
    
    // Format info text
    NSMutableString *infoText = [NSMutableString string];
    [infoText appendFormat:@"Wine iOS v%s\n", wineios_get_version()];
    [infoText appendFormat:@"Device: %@\n", deviceInfo];
    [infoText appendFormat:@"Jailbroken: %@\n", wineios_is_jailbroken() ? @"Yes" : @"No"];
    [infoText appendFormat:@"JIT: %@\n", wineios_has_jit_support() ? @"Enabled" : @"Disabled"];
    [infoText appendFormat:@"Prefix: %s\n", wineios_get_prefix()];
    [infoText appendString:@"\nNote: Full Windows app execution requires"];
    [infoText appendString:@"\ncross-compiled Wine ARM64 libraries."];
    
    infoLabel.text = infoText;
    
    // Add to main view
    UIView *mainView = self.window.rootViewController.view;
    [mainView addSubview:infoLabel];
    
    // Position in top-left with safe area
    UILayoutGuide *safeArea = mainView.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [infoLabel.leadingAnchor constraintEqualToAnchor:safeArea.leadingAnchor constant:16],
        [infoLabel.topAnchor constraintEqualToAnchor:safeArea.topAnchor constant:16],
        [infoLabel.widthAnchor constraintLessThanOrEqualToConstant:280]
    ]];
    
    // Auto-hide after 5 seconds
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        [UIView animateWithDuration:0.5 animations:^{
            infoLabel.alpha = 0;
        } completion:^(BOOL finished) {
            [infoLabel removeFromSuperview];
        }];
    });
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    WINE_TRACE("Wine iOS application resigning active\n");
    
    // Pause Wine if running
    if (self.wineProcessId > 0) {
        [self pauseWine];
    }
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    WINE_TRACE("Wine iOS application became active\n");
    
    // Resume Wine if it was paused
    if (self.wineProcessId > 0) {
        [self resumeWine];
    }
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    WINE_TRACE("Wine iOS application terminating\n");
    
    // Stop Wine gracefully
    [self stopWine];
}

#pragma mark - Wine Environment

- (NSString *)winePrefixPath
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return [paths.firstObject stringByAppendingPathComponent:@"wine_prefix"];
}

- (NSString *)wineCachePath
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    return [paths.firstObject stringByAppendingPathComponent:@"wine"];
}

- (NSString *)wineDocumentsPath
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return [paths.firstObject stringByAppendingPathComponent:@"WineDocuments"];
}

- (BOOL)isWineInitialized
{
    return _initialized;
}

- (BOOL)isJitEnabled
{
    return [WineJIT isJITEnabled];
}

- (BOOL)setupWineEnvironment
{
    WINE_TRACE("Setting up Wine environment\n");
    
    @autoreleasepool {
        // Create Wine prefix directory structure
        NSError *error = nil;
        NSFileManager *fm = [NSFileManager defaultManager];
        
        // Create directory structure
        NSArray *dirs = @[
            self.winePrefixPath,
            [self.winePrefixPath stringByAppendingPathComponent:@"dosdevices"],
            [self.winePrefixPath stringByAppendingPathComponent:@"drive_c"],
            [self.winePrefixPath stringByAppendingPathComponent:@"drive_c/windows"],
            [self.winePrefixPath stringByAppendingPathComponent:@"drive_c/program files"],
            self.wineCachePath,
            self.wineDocumentsPath
        ];
        
        for (NSString *dir in dirs) {
            if (![fm fileExistsAtPath:dir]) {
                if (![fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:&error]) {
                    WINE_ERR("Failed to create directory %@: %@\n", dir, error);
                    return NO;
                }
            }
        }
        
        // Create DOS device symlinks
        NSString *dosdevices = [self.winePrefixPath stringByAppendingPathComponent:@"dosdevices"];
        
        // c: -> drive_c
        NSString *cDrive = [dosdevices stringByAppendingPathComponent:@"c:"];
        if (![fm fileExistsAtPath:cDrive]) {
            [fm createSymbolicLinkAtPath:cDrive 
                       withDestinationPath:[self.winePrefixPath stringByAppendingPathComponent:@"drive_c"]
                                     error:&error];
        }
        
        // z: -> root
        NSString *zDrive = [dosdevices stringByAppendingPathComponent:@"z:"];
        if (![fm fileExistsAtPath:zDrive]) {
            [fm createSymbolicLinkAtPath:zDrive 
                       withDestinationPath:@"/"
                                     error:&error];
        }
        
        // Setup environment variables
        [self setupEnvironmentVariables];
        
        // Initialize JIT if available
        [WineJIT initializeJIT];
        
        _initialized = YES;
        
        WINE_TRACE("Wine environment setup complete\n");
    }
    
    return YES;
}

- (void)setupEnvironmentVariables
{
    // Set Wine environment variables
    setenv("WINEPREFIX", [self.winePrefixPath UTF8String], 1);
    setenv("TEMP", [self.wineCachePath UTF8String], 1);
    setenv("TMP", [self.wineCachePath UTF8String], 1);
    setenv("PATH", [[NSString stringWithFormat:@"%@/bin:/usr/bin:/bin:/usr/sbin:/sbin",
                     [[NSBundle mainBundle] resourcePath]] UTF8String], 1);
    
    // iOS-specific settings
    setenv("WINEDEBUG", "+ios,-relay", 1);
    setenv("SDL_VIDEODRIVER", "ios", 1);
    setenv("SDL_AUDIODRIVER", "aos", 1);
    
    // Disable sandbox restrictions for jailbroken devices
    setenv("JAILBREAK", "1", 1);
    
    WINE_TRACE("Environment variables set\n");
}

- (BOOL)startWine:(NSString *)executablePath
{
    WINE_TRACE("Starting Wine with executable: %@\n", executablePath);
    
    if (self.wineProcessId > 0) {
        WINE_WARN("Wine is already running (PID: %d)\n", self.wineProcessId);
        return NO;
    }
    
    dispatch_async(self.wineDispatchQueue, ^{
        @autoreleasepool {
            // Get path to Wine binary
            NSString *winePath = [[NSBundle mainBundle] pathForResource:@"wine" ofType:@""];
            if (!winePath) {
                // Fallback to known location
                winePath = @"/usr/local/bin/wine";
            }
            
            if (![[NSFileManager defaultManager] fileExistsAtPath:winePath]) {
                WINE_ERR("Wine binary not found at %@\n", winePath);
                return;
            }
            
            // Prepare arguments
            NSMutableArray *args = [NSMutableArray arrayWithObject:winePath];
            
            if (executablePath) {
                [args addObject:executablePath];
            } else {
                // Default to explorer
                [args addObject:@"explorer"];
            }
            
            // Convert to C array
            char *argv[args.count + 1];
            for (NSUInteger i = 0; i < args.count; i++) {
                argv[i] = (char *)[args[i] UTF8String];
            }
            argv[args.count] = NULL;
            
            // Fork and execute
            pid_t pid = fork();
            
            if (pid == 0) {
                // Child process
                execv([winePath UTF8String], argv);
                
                // If execv fails
                WINE_ERR("Failed to exec Wine: %s\n", strerror(errno));
                _exit(1);
            } else if (pid > 0) {
                // Parent process
                self.wineProcessId = pid;
                WINE_TRACE("Wine started with PID: %d\n", pid);
                
                // Wait for Wine to exit
                int status;
                waitpid(pid, &status, 0);
                
                WINE_TRACE("Wine exited with status: %d\n", WEXITSTATUS(status));
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    self.wineProcessId = 0;
                    // Notify UI that Wine has stopped
                    [[NSNotificationCenter defaultCenter] postNotificationName:@"WineProcessDidExit"
                                                                        object:nil
                                                                      userInfo:@{@"status": @(WEXITSTATUS(status))}];
                });
            } else {
                WINE_ERR("Failed to fork for Wine: %s\n", strerror(errno));
            }
        }
    });
    
    return YES;
}

- (BOOL)stopWine
{
    WINE_TRACE("Stopping Wine (PID: %d)\n", self.wineProcessId);
    
    if (self.wineProcessId <= 0) {
        WINE_WARN("Wine is not running\n");
        return NO;
    }
    
    // Send SIGTERM to Wine process
    if (kill(self.wineProcessId, SIGTERM) == 0) {
        // Wait for graceful shutdown
        int status;
        waitpid(self.wineProcessId, &status, WNOHANG);
        
        self.wineProcessId = 0;
        WINE_TRACE("Wine stopped successfully\n");
        return YES;
    } else {
        WINE_ERR("Failed to stop Wine: %s\n", strerror(errno));
        
        // Force kill if graceful shutdown fails
        kill(self.wineProcessId, SIGKILL);
        self.wineProcessId = 0;
        return NO;
    }
}

- (void)pauseWine
{
    if (self.wineProcessId > 0) {
        WINE_TRACE("Pausing Wine\n");
        kill(self.wineProcessId, SIGSTOP);
    }
}

- (void)resumeWine
{
    if (self.wineProcessId > 0) {
        WINE_TRACE("Resuming Wine\n");
        kill(self.wineProcessId, SIGCONT);
    }
}

#pragma mark - Window Management

- (WineViewController *)createWindowForHwnd:(NSInteger)hwnd
{
    WineViewController *vc = [[WineViewController alloc] initWithHwnd:hwnd];
    
    self.wineWindows[@(hwnd)] = vc;
    
    WINE_TRACE("Created WineViewController for HWND %ld\n", (long)hwnd);
    
    return vc;
}

#pragma mark - Event Handling

- (void)postEventToWine:(const void *)eventData size:(size_t)size
{
    [self.eventQueue pushEvent:eventData size:size];
}

#pragma mark - Notifications

- (void)didReceiveMemoryWarning:(NSNotification *)notification
{
    WINE_WARN("Received memory warning\n");
    
    // Clear caches
    [[NSFileManager defaultManager] removeItemAtPath:self.wineCachePath error:nil];
}

- (void)didChangeOrientation:(NSNotification *)notification
{
    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
    
    WINE_TRACE("Device orientation changed: %ld\n", (long)orientation);
    
    // Notify Wine of orientation change
    // This would be done via the event queue
}

- (CGFloat)screenScale
{
    return [UIScreen mainScreen].scale;
}

- (UIEdgeInsets)safeAreaInsets
{
    if (@available(iOS 11.0, *)) {
        return [UIApplication sharedApplication].windows.firstObject.safeAreaInsets;
    }
    return UIEdgeInsetsZero;
}

@end
