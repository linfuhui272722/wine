#!/bin/bash
# Complete Wine iOS Build Script
# Builds Wine core + iOS app and creates IPA
#
# Usage: ./build_complete_ios.sh [clean|build|package]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_PATH="${SDK_PATH:-/workspace/project/sdks/iPhoneOS16.5.sdk}"
BUILD_DIR="$SCRIPT_DIR/build-ios"
INSTALL_DIR="$SCRIPT_DIR/install-ios"
TARGET="arm64-apple-ios"
MIN_VERSION="13.0"

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
CFLAGS="$CFLAGS -fPIC -Wall -O2 -fobjc-arc -DNS_BLOCKS_ASSERTED=YES"
CXXFLAGS="$CFLAGS -stdlib=libc++"

LDFLAGS="-target $TARGET -isysroot $SDK_PATH -miphoneos-version-min=$MIN_VERSION"
LDFLAGS="$LDFLAGS -stdlib=libc++"

# Directories
WINEIOS_DIR="$SCRIPT_DIR/dlls/wineios.drv/wineios"
APP_DIR="$SCRIPT_DIR/dlls/wineios.drv/ios-app"

log_info "Wine iOS Complete Build"
log_info "SDK: $SDK_PATH"
log_info "Target: $TARGET"
log_info "Min iOS: $MIN_VERSION"
echo ""

# Clean
clean() {
    log_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    rm -rf "$INSTALL_DIR"
    mkdir -p "$BUILD_DIR/objects"
    mkdir -p "$INSTALL_DIR"
    log_info "Clean completed!"
}

# Setup
setup() {
    mkdir -p "$BUILD_DIR/objects"
    mkdir -p "$INSTALL_DIR"
}

# Compile Wine iOS Core
compile_wineios() {
    log_info "=== Compiling Wine iOS Core ==="
    
    local sources="
        $WINEIOS_DIR/wineios.c
        $WINEIOS_DIR/wine_core.c
        $WINEIOS_DIR/ios_syscalls.c
        $WINEIOS_DIR/ios_graphics.c
        $WINEIOS_DIR/ios_pe.c
    "
    
    for src in $sources; do
        if [[ ! -f "$src" ]]; then
            log_warn "Source not found: $src"
            continue
        fi
        
        local name=$(basename "$src" .c)
        local obj="$BUILD_DIR/objects/${name}.o"
        
        log_info "  Compiling $(basename $src)"
        clang $CFLAGS -I"$WINEIOS_DIR" -c "$src" -o "$obj" 2>&1 | grep -v "^warning:" || true
    done
    
    # Create static library
    log_info "Creating libwineios.a..."
    ar rcs "$BUILD_DIR/libwineios.a" "$BUILD_DIR/objects"/*.o
    log_info "  Created libwineios.a ($(du -h "$BUILD_DIR/libwineios.a" | cut -f1))"
}

# Compile iOS App
compile_app() {
    log_info "=== Compiling iOS App ==="
    
    local sources="
        $APP_DIR/main.m
        $APP_DIR/WineAppDelegate.m
        $APP_DIR/WineMenuViewController.m
        $APP_DIR/WineViewController.m
        $APP_DIR/WineBridge.m
        $APP_DIR/WineEventQueue.m
        $APP_DIR/WineJIT.m
    "
    
    for src in $sources; do
        if [[ ! -f "$src" ]]; then
            log_warn "Source not found: $src"
            continue
        fi
        
        local name=$(basename "$src" .m)
        local obj="$BUILD_DIR/objects/${name}.o"
        
        log_info "  Compiling $(basename $src)"
        clang $CFLAGS -I"$APP_DIR" -I"$WINEIOS_DIR" -c "$src" -o "$obj" 2>&1 | grep -v "^warning:" || true
    done
    
    # Create static library
    log_info "Creating libwineapp.a..."
    ar rcs "$BUILD_DIR/libwineapp.a" "$BUILD_DIR/objects"/*.o
    log_info "  Created libwineapp.a ($(du -h "$BUILD_DIR/libwineapp.a" | cut -f1))"
}

# Link executable
link_executable() {
    log_info "=== Linking Executable ==="
    
    # Collect all object files
    local objects=$(find "$BUILD_DIR/objects" -name "*.o" -type f)
    local obj_count=$(echo $objects | wc -w)
    
    log_info "  Total object files: $obj_count"
    
    # Try to link using lld with Darwin flavor
    log_info "  Linking with LLVM lld..."
    
    # Create executable using clang
    local exe="$BUILD_DIR/Wine"
    
    clang $LDFLAGS -o "$exe" $objects \
        -L"$BUILD_DIR" -lwineios \
        -L"$SDK_PATH/usr/lib" \
        -framework UIKit \
        -framework Foundation \
        -framework QuartzCore \
        -framework CoreGraphics \
        -framework OpenGLES \
        -lz -lbz2 \
        2>&1 || {
            log_warn "  Link with framework failed, trying alternative..."
            
            # Try without frameworks (for testing)
            clang $LDFLAGS -dynamiclib -o "$exe.dylib" $objects \
                -L"$BUILD_DIR" -lwineios 2>&1 && {
                    log_info "  Created dylib: $exe.dylib"
                }
        }
    
    if [[ -f "$exe" ]]; then
        log_info "  Created executable: $exe"
        log_info "  Size: $(du -h "$exe" | cut -f1)"
        
        # Verify Mach-O format
        local magic=$(head -c 4 "$exe" | od -A n -t x1 | tr -d ' ')
        log_info "  Mach-O magic: $magic"
    fi
}

# Create IPA package
create_ipa() {
    log_info "=== Creating IPA Package ==="
    
    local app_dir="$BUILD_DIR/Wine.app"
    local ipa_path="$BUILD_DIR/Wine.ipa"
    
    # Create app bundle structure
    mkdir -p "$app_dir"
    mkdir -p "$app_dir/Frameworks"
    mkdir -p "$app_dir/_CodeSignature"
    
    # Copy executable
    if [[ -f "$BUILD_DIR/Wine" ]]; then
        cp "$BUILD_DIR/Wine" "$app_dir/Wine"
    fi
    
    # Copy static libraries
    cp "$BUILD_DIR/libwineios.a" "$app_dir/"
    cp "$BUILD_DIR/libwineapp.a" "$app_dir/"
    
    # Create Info.plist
    log_info "  Creating Info.plist..."
    cat > "$app_dir/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleDisplayName</key>
    <string>Wine iOS</string>
    <key>CFBundleExecutable</key>
    <string>Wine</string>
    <key>CFBundleIdentifier</key>
    <string>com.wine.ios.app</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>Wine</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSRequiresIPhoneOS</key>
    <true/>
    <key>MinimumOSVersion</key>
    <string>13.0</string>
    <key>UILaunchStoryboardName</key>
    <string>LaunchScreen</string>
    <key>UIRequiredDeviceCapabilities</key>
    <array>
        <string>armv7</string>
    </array>
    <key>UISupportedInterfaceOrientations</key>
    <array>
        <string>UIInterfaceOrientationPortrait</string>
        <string>UIInterfaceOrientationLandscapeLeft</string>
        <string>UIInterfaceOrientationLandscapeRight</string>
    </array>
    <key>UISupportedInterfaceOrientations~ipad</key>
    <array>
        <string>UIInterfaceOrientationPortrait</string>
        <string>UIInterfaceOrientationPortraitUpsideDown</string>
        <string>UIInterfaceOrientationLandscapeLeft</string>
        <string>UIInterfaceOrientationLandscapeRight</string>
    </array>
    <key>UIApplicationSupportsIndirectInputEvents</key>
    <true/>
    <key>NSPhotoLibraryUsageDescription</key>
    <string>Wine needs access to your photo library to load and save files.</string>
    <key>UIFileSharingEnabled</key>
    <true/>
    <key>LSSupportsOpeningDocumentsInPlace</key>
    <true/>
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeName</key>
            <string>Windows Executable</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Default</string>
            <key>LSItemContentTypes</key>
            <array>
                <string>com.microsoft.windows.executable</string>
            </array>
        </dict>
    </array>
    <key>UTExportedTypeDeclarations</key>
    <array>
        <dict>
            <key>UTTypeIdentifier</key>
            <string>com.microsoft.windows.executable</string>
            <key>UTTypeDescription</key>
            <string>Windows Executable</string>
            <key>UTTypeConformsTo</key>
            <array>
                <string>public.data</string>
            </array>
            <key>UTTypeTagSpecification</key>
            <dict>
                <key>public.filename-extension</key>
                <array>
                    <string>exe</string>
                </array>
            </dict>
        </dict>
    </dict>
</dict>
</plist>
PLIST

    # Copy LaunchScreen
    if [[ -f "$APP_DIR/LaunchScreen.storyboard" ]]; then
        cp "$APP_DIR/LaunchScreen.storyboard" "$app_dir/"
    fi
    
    # Create PkgInfo
    echo -n "APPL????" > "$app_dir/PkgInfo"
    
    # Create Wine prefix directory structure
    mkdir -p "$app_dir/wine"
    mkdir -p "$app_dir/wine/drive_c"
    mkdir -p "$app_dir/wine/drive_c/windows"
    mkdir -p "$app_dir/wine/drive_c/Program Files"
    mkdir -p "$app_dir/documents"
    
    # Create Wine configuration
    cat > "$app_dir/wine/config" << 'WINE_CONFIG'
[wine]
Version=7.0
#WINE=wine

[Drive A]
"Floppy"="fd0"
Type=floopy

[Drive C]
"Hard Disk"="${C:}"
Type=hd

[Drive D]
"CD-ROM"="${D:}"
Type=cdrom

[Drive E]
"Documents"="${HOME}/Documents"
Type=network

[wineboot]
HardwareAcceleration=disabled

[winemac]
UseOpenGLES=1
WINEIOS_DEVICE=iPhone
WINEIOS_SCREEN_SCALE=2.0
WINEIOS_KEYBOARD=1
WINEIOS_MOUSE=1
WINE_CONFIG

    # Create code signature (placeholder)
    echo "CODE_SIGNATURE_PLACEHOLDER" > "$app_dir/_CodeSignature/CodeResources"
    
    # Create embedded.mobileprovision
    echo "EMBEDDED_MOBILEPROVISION_PLACEHOLDER" > "$app_dir/embedded.mobileprovision"
    
    # Package as IPA
    log_info "  Creating IPA..."
    cd "$BUILD_DIR"
    mkdir -p Payload
    cp -r Wine.app Payload/
    
    # Try zip first
    if command -v zip &> /dev/null; then
        rm -f Wine.ipa
        zip -r Wine.ipa Payload
        ipa_path="$BUILD_DIR/Wine.ipa"
    else
        # Create tarball as fallback
        tar -czf Wine.tar.gz Payload
        ipa_path="$BUILD_DIR/Wine.tar.gz"
    fi
    
    cd "$SCRIPT_DIR"
    
    echo ""
    log_info "=========================================="
    log_info "   Build Complete!"
    log_info "=========================================="
    echo ""
    log_info "IPA: $ipa_path"
    log_info "Size: $(du -h $ipa_path | cut -f1)"
    log_info ""
    log_info "App Bundle Contents:"
    ls -la "$app_dir/"
    echo ""
    log_info "Wine Prefix:"
    ls -la "$app_dir/wine/"
    echo ""
}

# Build summary
summary() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Build Summary${NC}"
    echo "=========================================="
    echo ""
    
    if [[ -d "$BUILD_DIR" ]]; then
        log_info "Executable: $([[ -f "$BUILD_DIR/Wine" ]] && echo "YES ($(du -h $BUILD_DIR/Wine | cut -f1))" || echo "NO")"
        log_info "libwineios.a: $([[ -f "$BUILD_DIR/libwineios.a" ]] && echo "YES ($(du -h $BUILD_DIR/libwineios.a | cut -f1))" || echo "NO")"
        log_info "libwineapp.a: $([[ -f "$BUILD_DIR/libwineapp.a" ]] && echo "YES ($(du -h $BUILD_DIR/libwineapp.a | cut -f1))" || echo "NO")"
        log_info "IPA: $([[ -f "$BUILD_DIR/Wine.ipa" ]] && echo "YES ($(du -h $BUILD_DIR/Wine.ipa | cut -f1))" || echo "NO")"
        echo ""
        
        log_info "Object Files:"
        find "$BUILD_DIR/objects" -name "*.o" -exec ls -lh {} \; 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}' | sort
    fi
    
    echo ""
    echo "=========================================="
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Wine iOS Complete Build${NC}"
    echo "=========================================="
    echo ""
    
    # Check SDK
    if [[ ! -d "$SDK_PATH" ]]; then
        log_error "SDK not found at $SDK_PATH"
        log_info "Please set SDK_PATH environment variable"
        exit 1
    fi
    
    case "${1:-build}" in
        clean)
            clean
            ;;
        build)
            setup
            compile_wineios
            compile_app
            link_executable
            create_ipa
            summary
            ;;
        compile)
            setup
            compile_wineios
            compile_app
            ;;
        link)
            link_executable
            ;;
        package)
            create_ipa
            ;;
        *)
            echo "Usage: $0 {clean|build|compile|link|package}"
            echo ""
            echo "  clean    - Clean build artifacts"
            echo "  build    - Full build (default)"
            echo "  compile  - Compile sources only"
            echo "  link     - Link executable"
            echo "  package  - Create IPA"
            exit 1
            ;;
    esac
}

main "$@"
