/*
 * WineViewController Implementation
 *
 * Copyright 2024 Wine Project
 */

#import "WineViewController.h"
#import "WineAppDelegate.h"
#import "WineEventQueue.h"
#import "WineBridge.h"

#include <sys/mman.h>
#include <pthread.h>

// OpenGL ES
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

// Wine debug channel (stubbed)
#define WINE_TRACE(...) do {} while(0)
#define WINE_WARN(...) do {} while(0)
#define WINE_ERR(...) do {} while(0)
#define WINE_DEFAULT_DEBUG_CHANNEL(x)

/* Touch tracking */
@interface WineTouchTracker : NSObject
@property (nonatomic, assign) UITouch *activeTouch;
@property (nonatomic, assign) NSInteger wineTouchId;
@property (nonatomic, assign) CGPoint startLocation;
@property (nonatomic, assign) CGPoint lastLocation;
@end

@implementation WineTouchTracker
@end

/* Wine view with Metal layer */
@interface WineMetalView : UIView
@property (nonatomic, strong) CAMetalLayer *metalLayer;
@end

@implementation WineMetalView
+ (Class)layerClass { return [CAMetalLayer class]; }
@end

/* Wine OpenGL view */
@interface WineOpenGLView : UIView
@property (nonatomic, strong) CAEAGLLayer *eaglLayer;
@end

@implementation WineOpenGLView
+ (Class)layerClass { return [CAEAGLLayer class]; }
@end

@interface WineViewController ()
{
    NSInteger _hwnd;
    UIView *_clientView;
    WineRenderingMode _renderingMode;
    
    // Metal
    id<MTLDevice> _metalDevice;
    id<MTLCommandQueue> _metalCommandQueue;
    CAMetalLayer *_metalLayer;
    
    // OpenGL
    EAGLContext *_eaglContext;
    CAEAGLLayer *_eaglLayer;
    GLuint _framebuffer;
    GLuint _renderbuffer;
    
    // Software rendering
    CGContextRef _cgContext;
    void *_surfaceBuffer;
    NSInteger _surfaceStride;
    NSInteger _surfaceFormat;
    
    // Touch tracking
    NSMutableDictionary<NSValue *, WineTouchTracker *> *_touches;
    NSInteger _nextTouchId;
    
    // Keyboard
    UIView *_inputAccessoryView;
    UITextField *_keyboardProxy;
    
    // State
    BOOL _touchEnabled;
    BOOL _keyboardEnabled;
    BOOL _visible;
    
    CADisplayLink *_displayLink;
    CGRect _windowRect;
    
    pthread_mutex_t _surfaceLock;
}

@property (nonatomic, strong) UIView *clientView;
@property (nonatomic, strong, readwrite, nullable) id<MTLDevice> metalDevice;
@property (nonatomic, strong, readwrite, nullable) id<MTLCommandQueue> metalCommandQueue;
@property (nonatomic, strong, readwrite, nullable) CAMetalLayer *metalLayer;
@property (nonatomic, strong, readwrite, nullable) EAGLContext *eaglContext;
@property (nonatomic, strong, readwrite, nullable) CAEAGLLayer *eaglLayer;
@property (nonatomic, strong, readwrite, nullable) CADisplayLink *displayLink;
@property (nonatomic, strong) NSMutableDictionary<NSValue *, WineTouchTracker *> *touches;
@property (nonatomic, assign) NSInteger nextTouchId;
@property (nonatomic, assign) CGSize surfaceSize;
@property (nonatomic, assign) NSInteger surfaceFormat;

@end

@implementation WineViewController

@synthesize hwnd = _hwnd;
@synthesize renderingMode = _renderingMode;
@synthesize visible = _visible;

- (instancetype)initWithHwnd:(NSInteger)hwnd
{
    self = [super init];
    if (self) {
        _hwnd = hwnd;
        _renderingMode = WineRenderingModeMetal;
        _touches = [NSMutableDictionary dictionary];
        _nextTouchId = 0;
        _touchEnabled = YES;
        _keyboardEnabled = YES;
        _visible = NO;
        
        pthread_mutex_init(&_surfaceLock, NULL);
        
        _windowRect = CGRectMake(0, 0, 320, 480); // Default size
    }
    return self;
}

- (void)dealloc
{
    [self cleanupRendering];
    pthread_mutex_destroy(&_surfaceLock);
    // ARC handles super dealloc automatically
}

#pragma mark - View Lifecycle

- (void)loadView
{
    [super loadView];
    
    // Create appropriate view based on rendering mode
    switch (_renderingMode) {
        case WineRenderingModeMetal:
            self.view = [[WineMetalView alloc] initWithFrame:self.view.bounds];
            break;
        case WineRenderingModeOpenGL:
            self.view = [[WineOpenGLView alloc] initWithFrame:self.view.bounds];
            break;
        default:
            self.view = [[UIView alloc] initWithFrame:self.view.bounds];
            break;
    }
    
    self.view.backgroundColor = [UIColor blackColor];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    
    // Create client view for content
    _clientView = [[UIView alloc] initWithFrame:self.view.bounds];
    _clientView.backgroundColor = [UIColor blackColor];
    _clientView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:_clientView];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Setup rendering based on mode
    [self setupRendering];
    
    // Start display link
    [self startDisplayLink];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    _visible = YES;
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    _visible = NO;
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    // Notify delegate of frame change
    if ([self.wineDelegate respondsToSelector:@selector(wineViewController:didChangeFrame:)]) {
        [self.wineDelegate wineViewController:self didChangeFrame:self.view.frame];
    }
}

#pragma mark - Rendering Setup

- (void)setupWithRenderingMode:(WineRenderingMode)mode
{
    _renderingMode = mode;
    
    if (self.isViewLoaded) {
        [self cleanupRendering];
        [self.view removeFromSuperview];
        self.view = nil;
        [self loadView];
    }
}

- (void)setupRendering
{
    switch (_renderingMode) {
        case WineRenderingModeMetal:
            [self setupMetalRendering];
            break;
        case WineRenderingModeOpenGL:
            [self setupOpenGLRendering];
            break;
        case WineRenderingModeCoreGraphics:
        case WineRenderingModeSoftware:
            [self setupSoftwareRendering];
            break;
    }
}

- (void)setupMetalRendering
{
    if (![CAMetalLayer class]) {
        WINE_WARN("Metal not available, falling back to software rendering\n");
        _renderingMode = WineRenderingModeSoftware;
        [self setupSoftwareRendering];
        return;
    }
    
    _metalDevice = MTLCreateSystemDefaultDevice();
    _metalCommandQueue = [_metalDevice newCommandQueue];
    
    WineMetalView *metalView = (WineMetalView *)self.view;
    _metalLayer = metalView.metalLayer;
    
    _metalLayer.device = _metalDevice;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = NO;
    _metalLayer.contentsScale = [UIScreen mainScreen].scale;
    _metalLayer.drawableSize = CGSizeMake(
        self.view.bounds.size.width * _metalLayer.contentsScale,
        self.view.bounds.size.height * _metalLayer.contentsScale
    );
    
    WINE_TRACE("Metal rendering initialized with device: %@\n", _metalDevice.name);
}

- (void)setupOpenGLRendering
{
    _eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!_eaglContext) {
        WINE_WARN("OpenGL ES 2.0 not available, falling back to software rendering\n");
        _renderingMode = WineRenderingModeSoftware;
        [self setupSoftwareRendering];
        return;
    }
    
    WineOpenGLView *glView = (WineOpenGLView *)self.view;
    _eaglLayer = glView.eaglLayer;
    _eaglLayer.contentsScale = [UIScreen mainScreen].scale;
    _eaglLayer.drawableProperties = @{
        kEAGLDrawablePropertyRetainedBacking: @YES,
        kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8
    };
    
    [EAGLContext setCurrentContext:_eaglContext];
    
    // Create framebuffer
    glGenFramebuffers(1, &_framebuffer);
    glGenRenderbuffers(1, &_renderbuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    
    [_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:_eaglLayer];
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderbuffer);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        WINE_ERR("Framebuffer not complete: 0x%x\n", status);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    [EAGLContext setCurrentContext:nil];
    
    WINE_TRACE("OpenGL ES rendering initialized\n");
}

- (void)setupSoftwareRendering
{
    // Software rendering uses Core Graphics
    WINE_TRACE("Software rendering initialized\n");
}

- (void)cleanupRendering
{
    [self stopDisplayLink];
    
    if (_eaglContext) {
        [EAGLContext setCurrentContext:_eaglContext];
        if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
        if (_renderbuffer) glDeleteRenderbuffers(1, &_renderbuffer);
        [EAGLContext setCurrentContext:nil];
        _eaglContext = nil;
    }
    
    if (_surfaceBuffer) {
        free(_surfaceBuffer);
        _surfaceBuffer = nil;
    }
    
    if (_cgContext) {
        CGContextRelease(_cgContext);
        _cgContext = nil;
    }
}

#pragma mark - Display Link

- (void)startDisplayLink
{
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkFired:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stopDisplayLink
{
    [_displayLink invalidate];
    _displayLink = nil;
}

- (void)displayLinkFired:(CADisplayLink *)displayLink
{
    if (!_visible) return;
    
    // Present any pending surface updates
    [self presentSurface];
}

- (void)presentSurface
{
    switch (_renderingMode) {
        case WineRenderingModeMetal:
            [self presentMetalSurface];
            break;
        case WineRenderingModeOpenGL:
            [self presentOpenGLSurface];
            break;
        default:
            break;
    }
}

- (void)presentMetalSurface
{
    if (!_metalLayer || !_surfaceBuffer) return;
    
    id<CAMetalDrawable> drawable = _metalLayer.nextDrawable;
    if (!drawable) return;
    
    id<MTLCommandBuffer> commandBuffer = [_metalCommandQueue commandBuffer];
    
    // Copy surface buffer to drawable texture
    // This is a simplified implementation - real version would use compute shader
    // or optimized memcpy for better performance
    
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

- (void)presentOpenGLSurface
{
    if (!_eaglContext || !_surfaceBuffer) return;
    
    [EAGLContext setCurrentContext:_eaglContext];
    
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    
    // Copy surface data to renderbuffer
    // Real implementation would use glTexImage2D or direct buffer rendering
    
    [_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#pragma mark - Surface Management

- (void)setNeedsDisplay
{
    [self.view setNeedsDisplay];
}

- (void)invalidateSurface
{
    pthread_mutex_lock(&_surfaceLock);
    
    // Reallocate surface buffer if needed
    size_t newSize = _surfaceStride * self.surfaceSize.height;
    if (!_surfaceBuffer || newSize > _surfaceStride * 1024 * 1024) { // Max 1MB
        free(_surfaceBuffer);
        _surfaceBuffer = malloc(newSize);
    }
    
    pthread_mutex_unlock(&_surfaceLock);
    
    [self setNeedsDisplay];
}

- (void)updateWindowRect:(CGRect)rect
{
    _windowRect = rect;
    self.view.frame = rect;
    
    // Update layer drawable size
    if (_metalLayer) {
        _metalLayer.drawableSize = CGSizeMake(
            rect.size.width * _metalLayer.contentsScale,
            rect.size.height * _metalLayer.contentsScale
        );
    }
}

- (void)setSurfaceData:(void *)buffer stride:(NSInteger)stride format:(NSInteger)format
{
    pthread_mutex_lock(&_surfaceLock);
    
    _surfaceStride = stride;
    _surfaceFormat = format;
    
    if (_surfaceBuffer && buffer) {
        memcpy(_surfaceBuffer, buffer, stride * self.surfaceSize.height);
    }
    
    pthread_mutex_unlock(&_surfaceLock);
    
    [self setNeedsDisplay];
}

#pragma mark - Touch Input

- (void)setTouchInputEnabled:(BOOL)enabled
{
    _touchEnabled = enabled;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (!_touchEnabled) return;
    
    for (UITouch *touch in touches) {
        NSValue *key = [NSValue valueWithPointer:(__bridge const void *)touch];
        WineTouchTracker *tracker = [[WineTouchTracker alloc] init];
        tracker.activeTouch = touch;
        tracker.wineTouchId = _nextTouchId++;
        tracker.startLocation = [touch locationInView:self.view];
        tracker.lastLocation = tracker.startLocation;
        
        _touches[key] = tracker;
        
        // Send touch event to Wine
        [self sendTouchEvent:tracker phase:UITouchPhaseBegan];
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (!_touchEnabled) return;
    
    for (UITouch *touch in touches) {
        NSValue *key = [NSValue valueWithPointer:(__bridge const void *)touch];
        WineTouchTracker *tracker = _touches[key];
        
        if (tracker) {
            tracker.lastLocation = [touch locationInView:self.view];
            [self sendTouchEvent:tracker phase:UITouchPhaseMoved];
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (!_touchEnabled) return;
    
    for (UITouch *touch in touches) {
        NSValue *key = [NSValue valueWithPointer:(__bridge const void *)touch];
        WineTouchTracker *tracker = _touches[key];
        
        if (tracker) {
            tracker.lastLocation = [touch locationInView:self.view];
            [self sendTouchEvent:tracker phase:UITouchPhaseEnded];
            [_touches removeObjectForKey:key];
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [self touchesEnded:touches withEvent:event];
}

- (void)sendTouchEvent:(WineTouchTracker *)tracker phase:(UITouchPhase)phase
{
    // Create event structure for Wine
    typedef struct {
        NSInteger touch_id;
        CGFloat x;
        CGFloat y;
        CGFloat pressure;
        NSInteger phase;
        NSTimeInterval timestamp;
    } WineTouchEventData;
    
    WineTouchEventData eventData = {
        .touch_id = tracker.wineTouchId,
        .x = tracker.lastLocation.x,
        .y = tracker.lastLocation.y,
        .pressure = tracker.activeTouch.force / tracker.activeTouch.maximumPossibleForce,
        .phase = phase,
        .timestamp = tracker.activeTouch.timestamp
    };
    
    // Post to Wine via bridge
    [[WineBridge sharedBridge] postTouchEvent:&eventData size:sizeof(eventData) forHwnd:_hwnd];
    
    if ([self.wineDelegate respondsToSelector:@selector(wineViewController:didReceiveTouchEvent:)]) {
        [self.wineDelegate wineViewController:self didReceiveTouchEvent:(UIEvent *)nil];
    }
}

#pragma mark - Keyboard Input

- (void)setKeyboardInputEnabled:(BOOL)enabled
{
    _keyboardEnabled = enabled;
}

- (void)showKeyboard:(BOOL)show animated:(BOOL)animated
{
    if (show) {
        // Create keyboard proxy
        if (!_keyboardProxy) {
            _keyboardProxy = [[UITextField alloc] initWithFrame:CGRectZero];
            _keyboardProxy.autocorrectionType = UITextAutocorrectionTypeNo;
            _keyboardProxy.autocapitalizationType = UITextAutocapitalizationTypeNone;
            _keyboardProxy.keyboardType = UIKeyboardTypeDefault;
            _keyboardProxy.returnKeyType = UIReturnKeyDefault;
            _keyboardProxy.delegate = (id<UITextFieldDelegate>)self;
            [self.view addSubview:_keyboardProxy];
        }
        
        [_keyboardProxy becomeFirstResponder];
    } else {
        [_keyboardProxy resignFirstResponder];
    }
}

- (void)textFieldDidBeginEditing:(UITextField *)textField
{
    // Keyboard shown
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
    // Keyboard hidden
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    // Forward key events to Wine
    for (NSUInteger i = 0; i < string.length; i++) {
        unichar c = [string characterAtIndex:i];
        [[WineBridge sharedBridge] postCharacterEvent:c modifiers:0 forHwnd:_hwnd];
    }
    
    return NO;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    // Post return key event
    [[WineBridge sharedBridge] postCharacterEvent:'\r' modifiers:0 forHwnd:_hwnd];
    return NO;
}

@end
