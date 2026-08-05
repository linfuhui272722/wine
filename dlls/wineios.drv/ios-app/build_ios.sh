#!/bin/bash
# Wine iOS Cross-Compilation Script for Linux
# Uses clang with iOS SDK to create Mach-O object files
#
# Usage: ./build_ios.sh [clean|build|lipoc]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_PATH="${SDK_PATH:-/workspace/project/sdks/iPhoneOS16.5.sdk}"
TARGET="arm64-apple-ios"
MIN_VERSION="${MIN_VERSION:-13.0}"
BUILD_DIR="$SCRIPT_DIR/build"
INSTALL_DIR="$SCRIPT_DIR/install"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Compiler flags
CFLAGS="-target $TARGET -isysroot $SDK_PATH -miphoneos-version-min=$MIN_VERSION"
CFLAGS="$CFLAGS -fobjc-arc -DNS_BLOCKS_ASSERTED=YES -fPIC"
CFLAGS="$CFLAGS -Wall -Wextra -Wno-objc-root-class"

LDFLAGS="-target $TARGET -isysroot $SDK_PATH -miphoneos-version-min=$MIN_VERSION"
LDFLAGS="$LDFLAGS -fobjc-arc -Wl,-dead_strip"

# Source files
APP_SOURCES="
    main.m
    WineAppDelegate.m
    WineMenuViewController.m
    WineViewController.m
    WineBridge.m
    WineEventQueue.m
    WineJIT.m
"

WINEIOS_SOURCES="
    ../wineios/wineios.c
    ../wineios/ios_syscalls.c
    ../wineios/ios_graphics.c
    ../wineios/ios_pe.c
"

# Check SDK
check_sdk() {
    if [[ ! -d "$SDK_PATH" ]]; then
        log_error "SDK not found at $SDK_PATH"
        log_info "Please set SDK_PATH environment variable or clone theos/sdks"
        exit 1
    fi
    
    if [[ ! -f "$SDK_PATH/System/Library/Frameworks/UIKit.framework/Headers/UIKit.h" ]]; then
        log_error "UIKit.h not found in SDK"
        exit 1
    fi
    
    log_info "Using SDK: $SDK_PATH"
}

# Clean build
clean() {
    log_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    rm -rf "$INSTALL_DIR"
    log_info "Clean completed!"
}

# Create directories
setup() {
    mkdir -p "$BUILD_DIR"
    mkdir -p "$INSTALL_DIR"
}

# Compile Wine iOS core library
compile_wineios() {
    log_info "=== Compiling Wine iOS Core Library ==="
    
    local OBJECTS=""
    
    for src in $WINEIOS_SOURCES; do
        local file=$(basename "$src" .c)
        local obj="$BUILD_DIR/${file}.o"
        
        log_info "  Compiling $src -> $obj"
        
        clang $CFLAGS -c "$SCRIPT_DIR/$src" -o "$obj"
        
        if [[ $? -eq 0 ]]; then
            OBJECTS="$OBJECTS $obj"
        else
            log_error "Failed to compile $src"
            return 1
        fi
    done
    
    # Create static library
    log_info "Creating libwineios.a..."
    ar rcs "$INSTALL_DIR/libwineios.a" $OBJECTS
    
    if [[ -f "$INSTALL_DIR/libwineios.a" ]]; then
        log_info "Created libwineios.a ($(du -h "$INSTALL_DIR/libwineios.a" | cut -f1))"
    fi
}

# Compile iOS App
compile_app() {
    log_info "=== Compiling Wine iOS App ==="
    
    local OBJECTS=""
    
    for src in $APP_SOURCES; do
        local file=$(basename "$src" .m)
        local obj="$BUILD_DIR/${file}.o"
        
        log_info "  Compiling $src -> $obj"
        
        clang $CFLAGS -I"$SCRIPT_DIR" -I"$SCRIPT_DIR/../wineios" -c "$SCRIPT_DIR/$src" -o "$obj"
        
        if [[ $? -eq 0 ]]; then
            OBJECTS="$OBJECTS $obj"
        else
            log_error "Failed to compile $src"
            return 1
        fi
    done
    
    # Create static library for app objects
    log_info "Creating libwineapp.a..."
    ar rcs "$INSTALL_DIR/libwineapp.a" $OBJECTS
    
    if [[ -f "$INSTALL_DIR/libwineapp.a" ]]; then
        log_info "Created libwineapp.a ($(du -h "$INSTALL_DIR/libwineapp.a" | cut -f1))"
    fi
    
    return 0
}

# Try linking (requires macOS toolchain, will fail on Linux)
try_link() {
    log_info "=== Attempting to Link ==="
    
    # Collect all object files
    local OBJECTS=$(find "$BUILD_DIR" -name "*.o" -type f)
    
    if [[ -z "$OBJECTS" ]]; then
        log_error "No object files found"
        return 1
    fi
    
    log_info "Object files: $(echo $OBJECTS | wc -w)"
    
    # Try to link with lld
    log_info "Attempting link with lld..."
    
    if lld $LDFLAGS -o "$INSTALL_DIR/Wine" $OBJECTS \
        -L"$INSTALL_DIR" -lwineios \
        -framework UIKit -framework Foundation -framework QuartzCore \
        -lz -lbz2 -lc++ 2>&1; then
        
        log_info "Link successful!"
        log_info "Binary created: $INSTALL_DIR/Wine"
        
    else
        log_warn "Linking failed (this is expected on Linux)"
        log_info "Object files compiled successfully."
        log_info "To complete the build, run on macOS:"
        echo ""
        echo "  cd $SCRIPT_DIR"
        echo "  xcodebuild -project Wine.xcodeproj -sdk iphoneos -configuration Release build"
        echo ""
        return 1
    fi
}

# Generate Xcode project
generate_xcodeproj() {
    log_info "=== Generating Xcode Project ==="
    
    cat > "$SCRIPT_DIR/Wine.xcodeproj/project.pbxproj" << 'PBXPROJ'
// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {};
	objectVersion = 56;
	objects = {

/* Begin PBXBuildFile section */
		001 /* main.m in Sources */ = {isa = PBXBuildFile; fileRef = 101 /* main.m */; };
		002 /* WineAppDelegate.m in Sources */ = {isa = PBXBuildFile; fileRef = 102 /* WineAppDelegate.m */; };
		003 /* WineMenuViewController.m in Sources */ = {isa = PBXBuildFile; fileRef = 103 /* WineMenuViewController.m */; };
		004 /* WineViewController.m in Sources */ = {isa = PBXBuildFile; fileRef = 104 /* WineViewController.m */; };
		005 /* WineBridge.m in Sources */ = {isa = PBXBuildFile; fileRef = 105 /* WineBridge.m */; };
		006 /* WineEventQueue.m in Sources */ = {isa = PBXBuildFile; fileRef = 106 /* WineEventQueue.m */; };
		007 /* WineJIT.m in Sources */ = {isa = PBXBuildFile; fileRef = 107 /* WineJIT.m */; };
		008 /* libwineios.a in Sources */ = {isa = PBXBuildFile; fileRef = 108 /* libwineios.a */; };
		009 /* LaunchScreen.storyboard in Resources */ = {isa = PBXBuildFile; fileRef = 201 /* LaunchScreen.storyboard */; };
		00A /* Info.plist in Resources */ = {isa = PBXBuildFile; fileRef = 202 /* Info.plist */; };
		00B /* Wine.entitlements in Resources */ = {isa = PBXBuildFile; fileRef = 203 /* Wine.entitlements */; };
/* End PBXBuildFile section */

/* Begin PBXFileReference section */
		101 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = main.m; path = main.m; sourceTree = "<group>"; };
		102 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineAppDelegate.m; path = WineAppDelegate.m; sourceTree = "<group>"; };
		103 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineMenuViewController.m; path = WineMenuViewController.m; sourceTree = "<group>"; };
		104 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineViewController.m; path = WineViewController.m; sourceTree = "<group>"; };
		105 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineBridge.m; path = WineBridge.m; sourceTree = "<group>"; };
		106 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineEventQueue.m; path = WineEventQueue.m; sourceTree = "<group>"; };
		107 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.objc; name = WineJIT.m; path = WineJIT.m; sourceTree = "<group>"; };
		108 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = archive.ar; name = libwineios.a; path = libwineios.a; sourceTree = "<group>"; };
		109 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineAppDelegate.h; path = WineAppDelegate.h; sourceTree = "<group>"; };
		10A = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineMenuViewController.h; path = WineMenuViewController.h; sourceTree = "<group>"; };
		10B = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineViewController.h; path = WineViewController.h; sourceTree = "<group>"; };
		10C = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineBridge.h; path = WineBridge.h; sourceTree = "<group>"; };
		10D = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineEventQueue.h; path = WineEventQueue.h; sourceTree = "<group>"; };
		10E = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineJIT.h; path = WineJIT.h; sourceTree = "<group>"; };
		10F = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file; name = WineiOS.h; path = WineiOS.h; sourceTree = "<group>"; };
		201 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = file.storyboard; name = LaunchScreen.storyboard; path = LaunchScreen.storyboard; sourceTree = "<group>"; };
		202 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; name = Info.plist; path = Info.plist; sourceTree = "<group>"; };
		203 = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; name = Wine.entitlements; path = Wine.entitlements; sourceTree = "<group>"; };
		301 = {isa = PBXFileReference; lastKnownFileType = wrapper.application; name = Wine.app; path = Wine.app; sourceTree = BUILT_PRODUCTS_DIR; };
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		601 = {isa = PBXFrameworksBuildPhase; buildActionMask = 2147483647; files = (); runOnlyForDeploymentPostprocessing = 0; };
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		801 = {isa = PBXGroup; children = (901 /* Sources */, 902 /* Resources */, 903 /* Products */); sourceTree = "<group>"; };
		901 = {isa = PBXGroup; children = (101, 102, 103, 104, 105, 106, 107, 108, 109, 10A, 10B, 10C, 10D, 10E, 10F); name = Sources; sourceTree = "<group>"; };
		902 = {isa = PBXGroup; children = (201, 202, 203); name = Resources; sourceTree = "<group>"; };
		903 = {isa = PBXGroup; children = (301); name = Products; sourceTree = "<group>"; };
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
		501 = {isa = PBXNativeTarget; buildConfigurationList = 801; buildPhases = (902, 601); buildRules = (); dependencies = (); name = Wine; productName = Wine; productReference = 301; productType = "com.apple.product-type.application"; };
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		1011 = {isa = PBXProject; buildConfigurationList = 802; compatibilityVersion = "Xcode 14.0"; developmentRegion = en; hasScannedForEncodings = 0; knownRegions = (en, Base); mainGroup = 801; productRefGroup = 903; projectDirPath = ""; projectRoot = ""; targets = (501); };
/* End PBXProject section */

/* Begin PBXSourcesBuildPhase section */
		902 = {isa = PBXSourcesBuildPhase; buildActionMask = 2147483647; files = (001, 002, 003, 004, 005, 006, 007, 008); runOnlyForDeploymentPostprocessing = 0; };
/* End PBXSourcesBuildPhase section */

/* Begin XCBuildConfiguration section */
		803 = {isa = XCBuildConfiguration; buildSettings = {ALWAYS_SEARCH_USER_PATHS = NO; CLANG_ENABLE_MODULES = YES; CLANG_ENABLE_OBJC_ARC = YES; CODE_SIGN_IDENTITY = ""; CODE_SIGN_STYLE = Manual; IPHONEOS_DEPLOYMENT_TARGET = 13.0; LD_RUNPATH_SEARCH_PATHS = ("$(inherited)", "@executable_path/Frameworks"); SDKROOT = iphoneos;}; name = Debug; };
		804 = {isa = XCBuildConfiguration; buildSettings = {ALWAYS_SEARCH_USER_PATHS = NO; CLANG_ENABLE_MODULES = YES; CLANG_ENABLE_OBJC_ARC = YES; CODE_SIGN_IDENTITY = ""; CODE_SIGN_STYLE = Manual; COPY_PHASE_STRIP = YES; IPHONEOS_DEPLOYMENT_TARGET = 13.0; LD_RUNPATH_SEARCH_PATHS = ("$(inherited)", "@executable_path/Frameworks"); SDKROOT = iphoneos; VALIDATE_PRODUCT = YES;}; name = Release; };
		805 = {isa = XCBuildConfiguration; buildSettings = {ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon; CODE_SIGN_ENTITLEMENTS = "$(SRCROOT)/Wine.entitlements"; CODE_SIGN_IDENTITY = ""; CODE_SIGN_STYLE = Manual; INFOPLIST_FILE = "$(SRCROOT)/Info.plist"; IPHONEOS_DEPLOYMENT_TARGET = 13.0; LD_RUNPATH_SEARCH_PATHS = ("$(inherited)", "@executable_path/Frameworks"); OTHER_LDFLAGS = ("-ObjC", "-lc++", "-lz"); PRODUCT_BUNDLE_IDENTIFIER = com.wine.ios.app; PRODUCT_NAME = Wine; TARGETED_DEVICE_FAMILY = "1,2";}; name = Debug; };
		806 = {isa = XCBuildConfiguration; buildSettings = {ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon; CODE_SIGN_ENTITLEMENTS = "$(SRCROOT)/Wine.entitlements"; CODE_SIGN_IDENTITY = ""; CODE_SIGN_STYLE = Manual; INFOPLIST_FILE = "$(SRCROOT)/Info.plist"; IPHONEOS_DEPLOYMENT_TARGET = 13.0; LD_RUNPATH_SEARCH_PATHS = ("$(inherited)", "@executable_path/Frameworks"); OTHER_LDFLAGS = ("-ObjC", "-lc++", "-lz"); PRODUCT_BUNDLE_IDENTIFIER = com.wine.ios.app; PRODUCT_NAME = Wine; TARGETED_DEVICE_FAMILY = "1,2";}; name = Release; };
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		801 = {isa = XCConfigurationList; buildConfigurations = (803, 804); defaultConfigurationIsVisible = 0; defaultConfigurationName = Release; };
		802 = {isa = XCConfigurationList; buildConfigurations = (805, 806); defaultConfigurationIsVisible = 0; defaultConfigurationName = Release; };
/* End XCConfigurationList section */
	};
	rootObject = 1011;
}
PBXPROJ

    log_info "Xcode project generated at $SCRIPT_DIR/Wine.xcodeproj"
}

# Package for macOS build
package() {
    log_info "=== Creating Package for macOS Build ==="
    
    local PKG_DIR="$SCRIPT_DIR/package"
    rm -rf "$PKG_DIR"
    mkdir -p "$PKG_DIR"
    
    # Copy source files
    cp -r "$SCRIPT_DIR"/*.m "$PKG_DIR/" 2>/dev/null || true
    cp -r "$SCRIPT_DIR"/*.h "$PKG_DIR/" 2>/dev/null || true
    cp -r "$SCRIPT_DIR"/*.plist "$PKG_DIR/" 2>/dev/null || true
    cp -r "$SCRIPT_DIR"/*.storyboard "$PKG_DIR/" 2>/dev/null || true
    cp -r "$SCRIPT_DIR"/*.entitlements "$PKG_DIR/" 2>/dev/null || true
    cp -r "$SCRIPT_DIR"/*.xcodeproj "$PKG_DIR/" 2>/dev/null || true
    
    # Copy libraries
    mkdir -p "$PKG_DIR/lib"
    cp "$INSTALL_DIR"/*.a "$PKG_DIR/lib/" 2>/dev/null || true
    
    # Copy build script
    cp "$SCRIPT_DIR/build_on_mac.sh" "$PKG_DIR/" 2>/dev/null || true
    
    log_info "Package created at $PKG_DIR"
    log_info "To build on macOS:"
    echo ""
    echo "  1. Copy the 'package' directory to your Mac"
    echo "  2. Run: cd package && ./build_on_mac.sh"
    echo ""
}

# Show build summary
summary() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Build Summary${NC}"
    echo "=========================================="
    echo ""
    
    if [[ -d "$INSTALL_DIR" ]]; then
        log_info "Libraries:"
        ls -la "$INSTALL_DIR"/*.a 2>/dev/null || echo "  (none)"
        echo ""
        log_info "Object files:"
        find "$BUILD_DIR" -name "*.o" -exec ls -lh {} \; 2>/dev/null | head -10 || echo "  (none)"
        echo ""
    fi
    
    echo "=========================================="
    echo ""
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Wine iOS Cross-Compilation${NC}"
    echo "=========================================="
    echo ""
    log_info "Target: $TARGET"
    log_info "Minimum iOS: $MIN_VERSION"
    log_info "SDK: $SDK_PATH"
    echo ""
    
    check_sdk
    
    case "${1:-build}" in
        clean)
            clean
            ;;
        build)
            setup
            compile_wineios || exit 1
            compile_app || exit 1
            summary
            ;;
        link)
            try_link
            ;;
        package)
            setup
            compile_wineios
            compile_app
            generate_xcodeproj
            package
            ;;
        xcodeproj)
            generate_xcodeproj
            ;;
        *)
            echo "Usage: $0 {clean|build|link|package|xcodeproj}"
            echo ""
            echo "  clean     - Clean all build artifacts"
            echo "  build     - Compile source files (default)"
            echo "  link      - Attempt to link (will fail on Linux)"
            echo "  package   - Create package for macOS build"
            echo "  xcodeproj - Generate Xcode project"
            exit 1
            ;;
    esac
}

main "$@"
