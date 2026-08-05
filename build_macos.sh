#!/bin/bash
# Wine iOS macOS Build Script
# Run this on macOS with Xcode installed to create the final IPA
#
# Usage: ./build_macos.sh [clean|build|all]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-ios"
SDK_PATH="${SDK_PATH:-$(xcrun --sdk iphoneos --show-sdk-path)}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check macOS
if [[ "$(uname)" != "Darwin" ]]; then
    log_error "This script must be run on macOS"
    exit 1
fi

# Check Xcode
if ! command -v xcodebuild &> /dev/null; then
    log_error "Xcode not found"
    exit 1
fi

log_info "Wine iOS Build (macOS)"
log_info "SDK: $SDK_PATH"
echo ""

# Clean
clean() {
    log_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    log_info "Clean completed!"
}

# Setup
setup() {
    mkdir -p "$BUILD_DIR"
    mkdir -p "$BUILD_DIR/objects"
}

# Compile
compile() {
    log_info "=== Compiling Wine iOS ==="
    
    local SDK="$SDK_PATH"
    local TARGET="arm64-apple-ios"
    local MIN_VERSION="13.0"
    
    local CFLAGS="-target $TARGET -isysroot $SDK -miphoneos-version-min=$MIN_VERSION"
    CFLAGS="$CFLAGS -fPIC -fobjc-arc -DNS_BLOCKS_ASSERTED=YES"
    CFLAGS="$CFLAGS -Wall -O2"
    
    local WINEIOS_DIR="$SCRIPT_DIR/dlls/wineios.drv/wineios"
    local APP_DIR="$SCRIPT_DIR/dlls/wineios.drv/ios-app"
    
    # Compile Wine iOS Core
    log_info "Compiling Wine iOS Core..."
    
    for src in "$WINEIOS_DIR"/*.c; do
        [[ -f "$src" ]] || continue
        local name=$(basename "$src" .c)
        log_info "  Compiling $name.c"
        clang $CFLAGS -I"$WINEIOS_DIR" -c "$src" -o "$BUILD_DIR/objects/${name}.o"
    done
    
    # Create libwineios.a
    ar rcs "$BUILD_DIR/libwineios.a" "$BUILD_DIR/objects"/*.o
    log_info "Created libwineios.a ($(du -h "$BUILD_DIR/libwineios.a" | cut -f1))"
    
    # Compile iOS App
    log_info "Compiling iOS App..."
    
    for src in "$APP_DIR"/*.m; do
        [[ -f "$src" ]] || continue
        local name=$(basename "$src" .m)
        log_info "  Compiling $name.m"
        clang $CFLAGS -I"$APP_DIR" -I"$WINEIOS_DIR" -c "$src" -o "$BUILD_DIR/objects/${name}.o"
    done
    
    # Create libwineapp.a
    ar rcs "$BUILD_DIR/libwineapp.a" "$BUILD_DIR/objects"/*.o
    log_info "Created libwineapp.a ($(du -h "$BUILD_DIR/libwineapp.a" | cut -f1))"
}

# Build with Xcode
xcodebuild_project() {
    log_info "=== Building with Xcode ==="
    
    cd "$SCRIPT_DIR/dlls/wineios.drv/ios-app"
    
    xcodebuild -project Wine.xcodeproj \
        -sdk iphoneos \
        -configuration Release \
        CODE_SIGN_IDENTITY="" \
        CODE_SIGNING_REQUIRED=NO \
        CODE_SIGNING_ALLOWED=NO \
        IPHONEOS_DEPLOYMENT_TARGET=13.0 \
        ARCHS=arm64 \
        build 2>&1 | tail -30
    
    # Find built app
    local APP_PATH=$(find ~/Library/Developer/Xcode/DerivedData -name "Wine.app" -type d 2>/dev/null | head -1)
    
    if [[ -n "$APP_PATH" && -d "$APP_PATH" ]]; then
        log_info "App built: $APP_PATH"
        cp -r "$APP_PATH" "$BUILD_DIR/Wine.app"
    fi
}

# Create IPA
create_ipa() {
    log_info "=== Creating IPA Package ==="
    
    local APP_DIR="$BUILD_DIR/Wine.app"
    
    if [[ ! -d "$APP_DIR" ]]; then
        log_error "App not found at $APP_DIR"
        exit 1
    fi
    
    # Create Wine prefix
    mkdir -p "$APP_DIR/wine/drive_c/windows"
    mkdir -p "$APP_DIR/wine/drive_c/Program Files"
    mkdir -p "$APP_DIR/documents"
    
    # Create Wine config
    cat > "$APP_DIR/wine/config" << 'WINE_CONFIG'
[wine]
Version=8.0
WINE=wine

[wineboot]
HardwareAcceleration=disabled

[winemac]
UseOpenGLES=1
WINEIOS_DEVICE=iPhone
WINE_CONFIG

    # Package as IPA
    cd "$BUILD_DIR"
    mkdir -p Payload
    cp -r Wine.app Payload/
    
    # Remove signature if exists
    rm -rf Payload/Wine.app/_CodeSignature
    
    # Create unsigned IPA
    zip -r Wine.ipa Payload
    
    cd "$SCRIPT_DIR"
    
    log_info "=========================================="
    log_info "   Build Complete!"
    log_info "=========================================="
    log_info ""
    log_info "IPA: $BUILD_DIR/Wine.ipa"
    log_info "Size: $(du -h "$BUILD_DIR/Wine.ipa" | cut -f1)"
    log_info ""
    log_info "App Bundle: $BUILD_DIR/Wine.app"
    log_info "  Executable: $(ls -la "$APP_DIR/Wine" 2>/dev/null | awk '{print $5}') bytes"
    log_info "  LibWineiOS: $(ls -la "$APP_DIR/libwineios.a" 2>/dev/null | awk '{print $5}') bytes"
    log_info "  LibWineApp: $(ls -la "$APP_DIR/libwineapp.a" 2>/dev/null | awk '{print $5}') bytes"
    log_info ""
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}   Wine iOS Build (macOS)${NC}"
    echo "=========================================="
    echo ""
    
    case "${1:-build}" in
        clean)
            clean
            ;;
        compile)
            setup
            compile
            ;;
        build)
            setup
            compile
            xcodebuild_project
            ;;
        xcode)
            xcodebuild_project
            ;;
        ipa)
            create_ipa
            ;;
        all)
            setup
            compile
            xcodebuild_project
            create_ipa
            ;;
        *)
            echo "Usage: $0 {clean|compile|build|xcode|ipa|all}"
            exit 1
            ;;
    esac
}

main "$@"
